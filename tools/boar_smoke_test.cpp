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

    for (int i = 0; i < 1200; ++i) {
        boar.update(1.0f / 60);
        assert(std::fabs(boar.getPosition().x - 100) < 73);
        assert(std::fabs(boar.getPosition().y - 86) < 0.01f);
    }
    // A nearby player behind either facing must not trigger a charge.
    boar.reset();
    hero.setPosition(150, 86);
    for (int i = 0; i < 45; ++i) boar.update(1.0f / 60);
    assert(!std::strcmp(boar.getState()->getName(), "Walk"));
    assert(boar.getVelocity().x == -28.0f);
    // Reach the left patrol boundary to turn naturally toward the right.
    hero.setPosition(-500, 76);
    boar.setPosition(27, 86);
    boar.update(1.0f / 60);
    boar.setPosition(100, 86);
    hero.setPosition(50, 86);
    for (int i = 0; i < 45; ++i) boar.update(1.0f / 60);
    assert(!std::strcmp(boar.getState()->getName(), "Walk"));
    assert(boar.getVelocity().x == 28.0f);
    hero.setPosition(150, 86);
    boar.update(1.0f / 60);
    assert(!std::strcmp(boar.getState()->getName(), "Run"));
    assert(boar.getVelocity().x == 100.0f);
    // Crossing behind an already charging boar also ends its aggro.
    hero.setPosition(50, 86);
    boar.update(1.0f / 60);
    assert(!std::strcmp(boar.getState()->getName(), "Walk"));
    assert(boar.getVelocity().x == 28.0f);

    boar.reset();
    hero.setPosition(40, 76);
    for (int i = 0; i < 45; ++i) boar.update(1.0f / 60);
    assert(!std::strcmp(boar.getState()->getName(), "Run"));
    assert(boar.getVelocity().x < 0);
    hero.setPosition(boar.getPosition().x, 76);
    boar.updateCombat(0.016f);
    assert(hero.getHealth() == 85);
    boar.updateCombat(0.1f);
    assert(hero.getHealth() == 85);
    boar.updateCombat(1.0f);
    assert(hero.getHealth() == 70);

    boar.reset();
    hero.setPosition(65, 76);
    hero.setState("Attack01");
    auto* attack = static_cast<Animation*>(hero.getRenderable());
    if (attack->getScale().x < 0) attack->mirror(true, false);
    boar.updateCombat(0.016f); // anticipation does not hit
    assert(std::strcmp(boar.getState()->getName(), "Hit"));
    for (int i = 0; i < 20 && attack->getCurrentFrameIndex() == 0; ++i) attack->update(0.016f);
    attack->mirror(true, false); // facing away misses
    boar.updateCombat(0.016f);
    assert(std::strcmp(boar.getState()->getName(), "Hit"));
    attack->mirror(true, false);
    boar.updateCombat(0.016f);
    assert(!std::strcmp(boar.getState()->getName(), "Hit"));
    assert(!boar.shouldCollideWith(floor));
    for (int i = 0; i < 90; ++i) boar.update(1.0f / 60);
    assert(!boar.getRenderable()->isVisible());
    boar.reset();
    assert(boar.shouldCollideWith(floor) && boar.getRenderable()->isVisible());
    assert(std::fabs(boar.getPosition().x - 100) < 0.01f);

    hero.setState("Idle");
    hero.setPosition(-500, 76);
    floorShape.setWidth(230); // right edge at x=130: turn before walking off
    floor.setPosition(0, 0);
    for (int i = 0; i < 1200; ++i) {
        boar.update(1.0f / 60);
        assert(boar.getPosition().x < 112);
        assert(std::fabs(boar.getPosition().y - 86) < 0.01f);
    }

    // A solid wall beside the floor must turn the boar when collision
    // resolution, rather than the predictive ledge probe, detects it.
    boar.reset();
    hero.setPosition(-500, 76);
    floorShape.setWidth(400);
    floor.setPosition(0, 0);
    Square wallShape(vector2(125, 40), 24, 60);
    Tile wall;
    wall.getState()->setCollidable(&wallShape);
    world.addObject("wall", &wall);
    world.addObject("boar", &boar);
    // Turn at the patrol boundary first; the player must be in front to aggro.
    boar.setPosition(27, 86);
    boar.update(0.61f);
    hero.setPosition(180, 86); // Make the boar charge right into the wall.
    boar.setPosition(85, 86);
    Debug::Mode.enable();
    Debug::dbgCollision = true;
    bool sawRightFacing = false;
    bool sawTurnedLeft = false;
    for (int i = 0; i < 60; ++i) {
        boar.update(1.0f / 60);
        collision.update(world, 1.0f / 60);
        for (const auto& shape : collision.getDebugShapes()) {
            if (shape.object == &boar) {
                auto* body = static_cast<Square*>(boar.getCollidable());
                assert(shape.min == body->getMin() && shape.max == body->getMax());
            }
        }
        if (boar.getRenderable()->getScale().x < 0.0f) {
            sawRightFacing = true;
        }
        else if (sawRightFacing && boar.getPosition().x < 112.0f) {
            sawTurnedLeft = true;
        }
    }
    const float wallContactX = boar.getPosition().x;
    assert(wallContactX < 112.0f);
    Debug::dbgCollision = false;
    Debug::Mode.disable();
    assert(sawTurnedLeft); // turned back to face left immediately
    const float previousX = boar.getPosition().x;
    for (int i = 0; i < 18; ++i) {
        boar.update(1.0f / 60);
        collision.update(world, 1.0f / 60);
    }
    assert(boar.getPosition().x < previousX || boar.getVelocity().x < 0.0f);
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
