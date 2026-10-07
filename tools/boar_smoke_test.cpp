// Headless behavior checks using real sprite-sheet dimensions and game objects.
#include "../src/FantasySideScroller/Boar.h"
#include "../src/FantasySideScroller/Character.h"
#include "../src/Animation.h"
#include "../src/Debug.h"
#include "../src/Kinematics2D.h"
#include "../src/ObjectManager.h"
#include "../src/Square.h"
#include "../src/Tile.h"
#include "../src/Kinematics2D.h"
#include "../src/FantasySideScroller/LevelManager.h"
#include "../src/GameState.h"
#include "stb/stb_image.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace {

bool sampleSupportFullScan(
    const ObjectManager& world,
    float x,
    float footY,
    float above,
    float below,
    float& support)
{
    support = std::numeric_limits<float>::max();
    bool found = false;
    for (const auto& entry : world.getObjects()) {
        auto* tile = dynamic_cast<Tile*>(entry.second);
        if (!tile || tile->isNonCollidingLayer()) continue;
        float y = 0.0f;
        if (Kinematics2D::sampleSupportY(tile->getCollidable(), x, y) &&
            y >= footY - above && y <= footY + below && y < support) {
            support = y;
            found = true;
        }
    }
    return found;
}

void compareMapSupportQueryToFullScan(
    const ObjectManager& world,
    const vector2& spawn)
{
    constexpr float kQueryEpsilon = 0.001f;
    constexpr float kAbove = 160.0f;
    constexpr float kBelow = 320.0f;
    const float footY = spawn.y + 14.0f;
    // Include negative coordinates, cell boundaries, and points well outside
    // the map so static grid misses cannot be mistaken for missing terrain.
    const float sampleX[] = {
        spawn.x,
        spawn.x - kQueryEpsilon,
        spawn.x + kQueryEpsilon,
        spawn.x - 32.0f,
        spawn.x + 32.0f,
        -500.0f,
        -2048.0f,
        4096.0f};

    size_t localCandidateCount = std::numeric_limits<size_t>::max();
    for (float x : sampleX) {
        std::vector<GameObject*> candidates;
        world.queryBounds(
            vector2(x - kQueryEpsilon, footY - kAbove),
            vector2(x + kQueryEpsilon, footY + kBelow),
            candidates);

        float queriedSupport = std::numeric_limits<float>::max();
        bool queried = false;
        for (GameObject* object : candidates) {
            auto* tile = dynamic_cast<Tile*>(object);
            if (!tile || tile->isNonCollidingLayer()) continue;
            float y = 0.0f;
            if (Kinematics2D::sampleSupportY(tile->getCollidable(), x, y) &&
                y >= footY - kAbove && y <= footY + kBelow && y < queriedSupport) {
                queriedSupport = y;
                queried = true;
            }
        }

        float scannedSupport = std::numeric_limits<float>::max();
        const bool scanned = sampleSupportFullScan(
            world, x, footY, kAbove, kBelow, scannedSupport);
        assert(queried == scanned);
        if (queried && scanned) assert(std::fabs(queriedSupport - scannedSupport) < 0.01f);
        localCandidateCount = std::min(localCandidateCount, candidates.size());
    }

    // The real map is large enough for a narrow local query to prune at least
    // one object. Keep this conditional so a deliberately tiny fixture map
    // still exercises the support equivalence checks above.
    if (world.numObjects() > 8) {
        assert(localCandidateCount < world.numObjects());
    }
}

} // namespace

class TestTexture : public ITexture {
    unsigned int width = 0, height = 0;
public:
    explicit TestTexture(const char* path) : ITexture(path) {
        int w = 0, h = 0, channels = 0;
        const bool loaded = stbi_info(path, &w, &h, &channels) != 0;
        assert(loaded);
        width = w;
        height = h;
    }
    unsigned int getWidth() const override { return width; }
    unsigned int getHeight() const override { return height; }
};
class TestRenderer : public IRenderer {
public:
    ITexture* createTexture(const char* path, Color = 0) override {
        auto* texture = new TestTexture(path);
        m_Textures.store(texture);
        return texture;
    }
    void initialize() override {}
    void shutdown() override {}
    void render() override {}
};
int main() {
    Square oneWayShape(vector2(0, 100), 100, 32);
    SurfaceTraits2D oneWayTraits;
    oneWayTraits.flags = SurfaceFlags::Solid | SurfaceFlags::Walkable | SurfaceFlags::OneWay;
    oneWayShape.setSurfaceTraits(oneWayTraits);
    Square playerShape(vector2(40, 50.7f), 20, 50);
    // A character whose feet are already just below the platform must pass
    // through while falling; the old resting tolerance could snap the
    // character onto the platform from underneath.
    assert(!Kinematics2D::shouldResolveAsOneWay(
        &oneWayShape, &playerShape, vector2(0, 0.1f), vector2(0, 0), 0.5f));
    // A real downward crossing from above must still land on the platform.
    playerShape.setMin(vector2(40, 50.3f));
    assert(Kinematics2D::shouldResolveAsOneWay(
        &oneWayShape, &playerShape, vector2(0, 0.5f), vector2(0, 0), 0.5f));

    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);
    Engine2D::getEventSystem()->initialize(INFINITE);
    ObjectManager world;
    CollisionSystem collision;
    Square floorShape(vector2(-100, 100), 400, 32);
    Tile floor;
    floor.getState()->setCollidable(&floorShape);
    world.addObject("floor", &floor);
    Character hero;
    hero.setState("Idle");
    hero.setPosition(-500, 76);
    Boar boar(world, hero, vector2(100, 70));
    assert(std::fabs(boar.getPosition().y - 86) < 0.01f);
    assert(static_cast<Animation*>(boar.getState("Idle")->getRenderable())->getFrameCount() == 4);
    assert(static_cast<Animation*>(boar.getState("Run")->getRenderable())->getFrameCount() == 6);

    // A cached collider must be rebuilt when the renderable flips. The boar
    // body is intentionally offset, so the two orientations have different
    // world-space bounds.
    auto* boarAnimation = static_cast<Animation*>(boar.getRenderable());
    boarAnimation->setScale(vector2(1.0f, 1.0f));
    auto* leftBody = static_cast<Square*>(boar.getCollidable());
    const vector2 leftMin = leftBody->getMin();
    const vector2 leftMax = leftBody->getMax();
    const vector2 pivot = boarAnimation->getCurrentFrame()->getSprite()->getCenter();
    assert(pivot.x == 24.0f && pivot.y == 16.0f);
    boarAnimation->mirror(true, false);
    // Match the renderer's local vertex transform: scale * (vertex - pivot).
    // A centered 48px frame must still occupy [-24, 24] after mirroring.
    for (auto* frame : boarAnimation->getFrames()) {
        auto* sprite = frame->getSprite();
        assert(sprite->getCenter() == pivot);
        assert(sprite->getScale().x * (0.0f - pivot.x) == 24.0f);
        assert(sprite->getScale().x * (48.0f - pivot.x) == -24.0f);
    }
    auto* rightBody = static_cast<Square*>(boar.getCollidable());
    const vector2 rightMin = rightBody->getMin();
    const vector2 rightMax = rightBody->getMax();
    assert(leftMax.x - leftMin.x == rightMax.x - rightMin.x);
    assert(leftMin.x > rightMin.x && leftMax.x > rightMax.x);
    boarAnimation->mirror(true, false);
    assert(boarAnimation->getCurrentFrame()->getSprite()->getCenter() == pivot);
    auto* restoredBody = static_cast<Square*>(boar.getCollidable());
    assert(restoredBody->getMin() == leftMin && restoredBody->getMax() == leftMax);
    assert(rightMin.x == 2 * boar.getPosition().x - leftMax.x);
    assert(rightMax.x == 2 * boar.getPosition().x - leftMin.x);

    constexpr float dt = 1.0f / 60.0f;
    auto tickBoar = [&](int frames) {
        for (int i = 0; i < frames; ++i) boar.update(dt);
    };
    auto assertState = [&](const char* name) {
        if (std::strcmp(boar.getState()->getName(), name)) {
            std::cerr << "Expected " << name << ", got " << boar.getState()->getName()
                      << " at x=" << boar.getPosition().x
                      << " with vx=" << boar.getVelocity().x << std::endl;
        }
        assert(!std::strcmp(boar.getState()->getName(), name));
    };
    auto assertStationary = [&](float x) {
        assertState("Idle");
        assert(boar.getVelocity().x == 0.0f);
        assert(std::fabs(boar.getPosition().x - x) < 0.01f);
    };
    auto advanceStrike = [&](Animation* attack) {
        for (int i = 0; i < 20 && attack->getCurrentFrameIndex() == 0; ++i)
            attack->update(dt);
        assert(attack->getCurrentFrameIndex() > 0);
    };
    auto startPlayerAttack = [&](const char* name, bool facesLeft) -> Animation* {
        hero.setState("Idle");
        hero.setState(name);
        auto* attack = static_cast<Animation*>(hero.getRenderable());
        if ((attack->getScale().x < 0.0f) != facesLeft)
            attack->mirror(true, false);
        return attack;
    };

    // An undetected boar waits two seconds before patrolling.
    boar.reset();
    hero.setState("Idle");
    hero.setPosition(-500, 76);
    tickBoar(114); // 1.9 seconds: still inside the initial wait.
    assertStationary(100.0f);
    tickBoar(12); // 2.1 seconds: patrol has begun.
    assertState("Walk");
    assert(boar.getVelocity().x == -28.0f);

    // Patrol follows the actual floor, not a radius around the spawn.
    bool visitedLeftEdge = false;
    bool visitedRightEdge = false;
    for (int i = 0; i < 2400; ++i) {
        boar.update(dt);
        const float x = boar.getPosition().x;
        assert(x > -81.0f && x < 281.0f); // Floor is [-100, 300].
        assert(std::fabs(boar.getPosition().y - 86.0f) < 0.01f);
        visitedLeftEdge |= x < -70.0f;
        visitedRightEdge |= x > 270.0f;
    }
    assert(visitedLeftEdge && visitedRightEdge);

    // A nearby player behind a fresh left-facing boar does not provoke it.
    boar.reset();
    hero.setPosition(150, 86);
    tickBoar(114);
    assertStationary(100.0f);
    tickBoar(12);
    assertState("Walk");
    assert(boar.getVelocity().x == -28.0f);

    // At a real ledge it waits two seconds, turns, then waits two more.
    hero.setPosition(-500, 76);
    boar.setPosition(-80, 86); // Ahead probe is outside the floor at x < -100.
    boar.update(dt);
    assertStationary(-80.0f);
    assert(boar.getRenderable()->getScale().x > 0.0f); // Art faces left.
    tickBoar(114);
    assertStationary(-80.0f);
    assert(boar.getRenderable()->getScale().x > 0.0f);
    tickBoar(12);
    assertStationary(-80.0f);
    assert(boar.getRenderable()->getScale().x < 0.0f); // Turned right.
    tickBoar(108); // 3.9 seconds since stopping: post-turn wait remains.
    assertStationary(-80.0f);
    tickBoar(12); // 4.1 seconds: rightward patrol resumes.
    assertState("Walk");
    assert(boar.getVelocity().x == 28.0f);

    // Ordinary sight detection still requires the player to be in front.
    boar.setPosition(100, 86);
    hero.setPosition(50, 86);
    boar.update(dt);
    assertState("Walk");
    assert(boar.getVelocity().x == 28.0f);
    hero.setPosition(180, 86);
    boar.update(dt);
    assertState("Run");
    assert(boar.getVelocity().x == 100.0f);
    hero.setPosition(50, 86);
    boar.update(dt);
    assertState("Walk");
    assert(boar.getVelocity().x == 28.0f); // Never damaged: no aggro latch.

    // A real sword hit from behind provokes a turn and a charge.
    boar.reset();
    hero.resetForRespawn();
    hero.setPosition(135, 76);
    auto* attackBehind = startPlayerAttack("Attack01", true);
    boar.updateCombat(dt); // Anticipation frame cannot damage/provoke.
    assert(!boar.getRenderable()->isFlashing());
    advanceStrike(attackBehind);
    boar.updateCombat(dt);
    assert(boar.getRenderable()->isFlashing());
    assert(boar.getRenderable()->isVisible());
    assert(boar.shouldCollideWith(floor)); // First hit is nonfatal.
    hero.setState("Idle");
    hero.setPosition(180, 76); // Clear separation makes charge direction unambiguous.
    boar.updateCombat(dt); // Leave Attack01 to arm the next slash.
    tickBoar(24); // Beyond the 0.25-second detection reaction.
    assertState("Run");
    assert(boar.getRenderable()->getScale().x < 0.0f);
    assert(boar.getVelocity().x == 100.0f);
    hero.setPosition(boar.getPosition().x - 70.0f, 76);
    boar.update(dt);
    assertState("Run");
    assert(boar.getRenderable()->getScale().x > 0.0f);
    assert(boar.getVelocity().x == -100.0f); // Damage aggro tracks a crossing target.

    // Respawn clears damage aggro and restores the initial patrol wait.
    boar.reset();
    hero.setPosition(150, 86);
    assert(!boar.getRenderable()->isFlashing());
    tickBoar(114);
    assertStationary(100.0f);
    tickBoar(12);
    assertState("Walk");
    assert(boar.getVelocity().x == -28.0f);

    // Three landed hits clear damage aggro: a player who then steps behind
    // the boar is no longer tracked.
    boar.reset();
    hero.resetForRespawn();
    hero.setPosition(135, 76);
    auto* provoke = startPlayerAttack("Attack01", true);
    advanceStrike(provoke);
    boar.updateCombat(dt);
    assert(boar.getRenderable()->isFlashing());
    hero.setState("Idle");
    boar.updateCombat(dt);
    tickBoar(120); // Turn, charge, and settle at the attack gap.
    assert(boar.getRenderable()->getScale().x < 0.0f); // Facing right.
    for (int hit = 1; hit <= 3; ++hit) {
        boar.update(dt);
        boar.updateCombat(1.0f);
        assert(hero.getHealth() == 100.0f - 10.0f * hit);
    }
    hero.setPosition(boar.getPosition().x - 60.0f, 76);
    tickBoar(60);
    assert(boar.getRenderable()->getScale().x < 0.0f); // Did not turn around.
    assert(std::strcmp(boar.getState()->getName(), "Run") != 0);
    assert(boar.getVelocity().x >= 0.0f);

    // Damage aggro expires after ten seconds without another hit.
    auto provokeThenWait = [&](int frames) {
        boar.reset();
        hero.resetForRespawn();
        hero.setPosition(135, 76);
        auto* slash = startPlayerAttack("Attack01", true);
        advanceStrike(slash);
        boar.updateCombat(dt);
        assert(boar.getRenderable()->isFlashing());
        hero.setState("Idle");
        boar.updateCombat(dt);
        hero.setPosition(-500, 76); // Out of detection while the timer runs.
        tickBoar(frames);
        // Step behind the boar, whichever way its patrol left it facing.
        const bool facingRight = boar.getRenderable()->getScale().x < 0.0f;
        hero.setPosition(boar.getPosition().x + (facingRight ? -60.0f : 60.0f), 76);
        boar.update(dt);
        return facingRight != (boar.getRenderable()->getScale().x < 0.0f);
    };
    assert(provokeThenWait(570));   // 9.5 seconds: still aggro, turns to track.
    assert(!provokeThenWait(606));  // 10.1 seconds: calm, ignores the player behind.

    // A charge stops at a gap before attacking; body overlap is unnecessary.
    boar.reset();
    hero.resetForRespawn();
    hero.setState("Idle");
    hero.setPosition(40, 76);
    bool sawCharge = false;
    for (int i = 0; i < 120; ++i) {
        boar.update(dt);
        sawCharge |= !std::strcmp(boar.getState()->getName(), "Run");
    }
    assert(sawCharge);
    assertState("Idle");
    assert(boar.getVelocity().x == 0.0f);
    vector2 heroMin, heroMax, enemyMin, enemyMax;
    assert(Kinematics2D::tryGetActiveBounds(hero.getCollidable(), heroMin, heroMax));
    assert(Kinematics2D::tryGetActiveBounds(boar.getCollidable(), enemyMin, enemyMax));
    assert(std::fabs(enemyMin.x - heroMax.x - 4.0f) < 0.6f);
    const float attackPosition = boar.getPosition().x;
    boar.updateCombat(dt);
    assert(hero.getHealth() == 90.0f);
    assertState("Attack");
    boar.update(dt);
    assert(std::fabs(boar.getPosition().x - attackPosition) < 0.01f);
    boar.updateCombat(0.1f);
    assert(hero.getHealth() == 90.0f);
    boar.updateCombat(0.91f);
    assert(hero.getHealth() == 80.0f);

    // Directional sword reach, per-slash counting, flash, and two-hit defeat.
    boar.reset();
    hero.resetForRespawn();
    hero.setPosition(65, 76);
    auto* attack = startPlayerAttack("Attack01", false);
    boar.updateCombat(dt);
    assert(!boar.getRenderable()->isFlashing());
    advanceStrike(attack);
    attack->mirror(true, false); // Facing away misses.
    boar.updateCombat(dt);
    assert(!boar.getRenderable()->isFlashing());
    attack->mirror(true, false);
    boar.updateCombat(dt);
    assert(boar.getRenderable()->isFlashing());
    assert(boar.getRenderable()->isVisible() && boar.shouldCollideWith(floor));
    boar.updateCombat(dt); // Same slash cannot count twice.
    assert(boar.getRenderable()->isVisible() && boar.shouldCollideWith(floor));
    hero.setState("Idle");
    boar.updateCombat(dt);
    auto* secondAttack = startPlayerAttack("Attack02", false);
    advanceStrike(secondAttack);
    boar.updateCombat(dt);
    assert(!boar.getRenderable()->isVisible());
    assert(!boar.shouldCollideWith(floor));
    assert(boar.getVelocity().x == 0.0f && boar.getVelocity().y == 0.0f);
    tickBoar(90);
    assert(!boar.getRenderable()->isVisible());
    boar.reset();
    assert(boar.shouldCollideWith(floor) && boar.getRenderable()->isVisible());
    assert(!boar.getRenderable()->isFlashing());
    assert(std::fabs(boar.getPosition().x - 100.0f) < 0.01f);

    hero.setState("Idle");
    hero.setPosition(-500, 76);
    floorShape.setWidth(230); // Right edge at x=130: turn before walking off.
    floor.setPosition(0, 0);
    for (int i = 0; i < 1200; ++i) {
        boar.update(dt);
        assert(boar.getPosition().x > -81.0f && boar.getPosition().x < 112.0f);
        assert(std::fabs(boar.getPosition().y - 86.0f) < 0.01f);
    }

    // A solid wall stops the boar through collision resolution, then follows
    // the same two-second pre-turn and two-second post-turn waits.
    boar.reset();
    hero.setPosition(-500, 76);
    floorShape.setWidth(400);
    floor.setPosition(0, 0);
    tickBoar(126);
    boar.setPosition(-80, 86);
    boar.update(dt);
    tickBoar(252);
    assert(boar.getRenderable()->getScale().x < 0.0f);
    assert(boar.getVelocity().x == 28.0f);
    boar.setPosition(85, 86);
    hero.setPosition(180, 86);
    Square wallShape(vector2(125, 40), 24, 60);
    Tile wall;
    wall.getState()->setCollidable(&wallShape);
    world.addObject("wall", &wall);
    world.addObject("boar", &boar);
    Debug::Mode.enable();
    Debug::dbgCollision = true;
    auto tickWorld = [&](int frames) {
        for (int i = 0; i < frames; ++i) {
            boar.update(dt);
            collision.update(world, dt);
            for (const auto& shape : collision.getDebugShapes()) {
                if (shape.object == &boar) {
                    auto* body = static_cast<Square*>(boar.getCollidable());
                    assert(shape.min == body->getMin() && shape.max == body->getMax());
                }
            }
        }
    };
    bool stoppedAtWall = false;
    for (int i = 0; i < 60; ++i) {
        tickWorld(1);
        if (boar.getPosition().x > 85.0f && boar.getVelocity().x == 0.0f &&
            !std::strcmp(boar.getState()->getName(), "Idle")) {
            stoppedAtWall = true;
            break;
        }
    }
    assert(stoppedAtWall);
    const float wallContactX = boar.getPosition().x;
    assert(wallContactX < 112.0f);
    assert(boar.getRenderable()->getScale().x < 0.0f);
    tickWorld(114);
    assertStationary(wallContactX);
    assert(boar.getRenderable()->getScale().x < 0.0f);
    tickWorld(12);
    assertState("Idle");
    assert(boar.getVelocity().x == 0.0f);
    assert(boar.getRenderable()->getScale().x > 0.0f);
    // Mirroring the offset body expands its right edge by four pixels.
    // The solver may shift the center left to keep the new body outside the wall.
    const float turnedWallX = boar.getPosition().x;
    assert(turnedWallX <= wallContactX + 0.01f);
    assert(turnedWallX >= wallContactX - 4.1f);
    auto* turnedWallBody = static_cast<Square*>(boar.getCollidable());
    assert(turnedWallBody->getMax().x <= 125.01f);
    tickWorld(108);
    assertStationary(turnedWallX);
    tickWorld(12);
    assertState("Walk");
    assert(boar.getVelocity().x == -28.0f);
    assert(boar.getPosition().x < turnedWallX);
    Debug::dbgCollision = false;
    Debug::Mode.disable();
    world.removeObject(&boar);
    world.removeObject(&wall);

    {
        ObjectManager mapWorld;
        GameState state;
        LevelManager level;
        level.initialize("mosswood_hollow.tmj", vector2(-60, 0), "Background/Background.png", mapWorld, state);
        assert(level.hasSpawnPoint());
        compareMapSupportQueryToFullScan(mapWorld, level.getSpawnPoint());
        hero.setPosition(level.getSpawnPoint());
        Boar mapBoar(mapWorld, hero, level.getSpawnPoint() + vector2(140, 10));
        const vector2 spawn = mapBoar.getPosition();
        assert(spawn.y < level.getSpawnPoint().y + 100);
        mapWorld.addObject("Boar", &mapBoar);
        for (int i = 0; i < 300; ++i) {
            mapBoar.update(1.0f / 60);
            collision.update(mapWorld, 1.0f / 60);
            assert(mapBoar.getPosition().y < spawn.y + 30);
        }
        std::cout << "Map spawn: " << spawn.x << ", " << spawn.y << "\n";
        collision.reset();
        mapWorld.removeObject(&mapBoar);
        level.shutdown(mapWorld, state);
    }
    std::cout << "PASS: real-map terrain, assets, spawn, patrol, charge, cooldown, directional attack, defeat, reset, ledge avoidance\n";
}
