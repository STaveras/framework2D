// Headless checks for tile-driven props (keys, chests, sinking lily pads) and for hazard
// knockback, using the real fantasyTiles tileset and Character.
#include "../src/FantasySideScroller/Character.h"
#include "../src/FantasySideScroller/LevelProps.h"
#include "../src/FantasySideScroller/TraversalMechanics.h"
#include "../src/Engine2D.h"
#include "../src/Kinematics2D.h"
#include "../src/System.h"
#include "../src/TileMap.h"
#include "stb/stb_image.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

namespace {

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

bool near(float a, float b, float epsilon = 0.01f) { return std::fabs(a - b) <= epsilon; }

void bodyBounds(Character& hero, vector2& outMin, vector2& outMax)
{
    const bool hasBody = Kinematics2D::tryGetActiveBounds(hero.getCollidable(), outMin, outMax);
    assert(hasBody);
}

// Moves the hero so its body is centred on a point.
void centreBodyOn(Character& hero, vector2 point)
{
    vector2 min, max;
    bodyBounds(hero, min, max);
    hero.setPosition(hero.getPosition() + point - (min + max) * 0.5f);
}

// Moves the hero so its feet rest on y, centred horizontally on x.
void standAt(Character& hero, float x, float y)
{
    vector2 min, max;
    bodyBounds(hero, min, max);
    hero.setPosition(hero.getPosition() + vector2(x - (min.x + max.x) * 0.5f, y - max.y));
}

vector2 tileCentre(Tile* tile) { return tile->getPosition() + vector2(8.0f, 8.0f); }

const LevelProps::SinkingPlatform& platformContaining(const LevelProps& props, Tile* part)
{
    for (const auto& platform : props.getPlatforms()) {
        for (Tile* tile : platform.tiles) {
            if (tile == part) {
                return platform;
            }
        }
    }
    assert(false && "pad part must belong to a platform");
    return props.getPlatforms().front();
}

void assertPlatformDepth(const LevelProps::SinkingPlatform& platform, float depth)
{
    assert(near(platform.depth, depth));
    assert(platform.tiles.size() == platform.restPositions.size());
    for (size_t i = 0; i < platform.tiles.size(); ++i) {
        assert(near(platform.tiles[i]->getPosition().x, platform.restPositions[i].x));
        assert(near(platform.tiles[i]->getPosition().y, platform.restPositions[i].y + depth));
    }
}

void placePad(TileMap& map, TileSet* tiles, unsigned int x, unsigned int y,
    int topLeft, int bottomLeft, unsigned int flipFlags)
{
    const bool mirrored = (flipFlags & TileSet::kFlipHorizontal) != 0;
    map.setTile(x, y, tiles, topLeft + (mirrored ? 1 : 0), flipFlags);
    map.setTile(x + 1, y, tiles, topLeft + (mirrored ? 0 : 1), flipFlags);
    map.setTile(x, y + 1, tiles, bottomLeft + (mirrored ? 1 : 0), flipFlags);
    map.setTile(x + 1, y + 1, tiles, bottomLeft + (mirrored ? 0 : 1), flipFlags);
}

void checkAdjacentPads(TileSet* tiles, Character& hero)
{
    TileLayerConfig config;
    config.name = "adjacent pads";
    config.collisionMode = TileCollisionMode::OneWay;
    hero.setState("Idle");
    hero.setVelocity(vector2(0.0f, 0.0f));

    for (int rightVariant : {492, 494}) {
        for (unsigned int flipFlags : {0u, TileSet::kFlipHorizontal}) {
            TileMap pads(8, 5, tiles, config);
            placePad(pads, tiles, 2, 2, 492, 517, flipFlags);
            // The level also uses the second upper variant with the first variant's
            // lower artwork. Those four cells must still make one complete pad.
            placePad(pads, tiles, 4, 2, rightVariant, 517, flipFlags);
            pads.arrangeTiles();

            LevelProps props;
            props.initialize({&pads});
            assert(props.getPlatforms().size() == 2);
            const auto& left = platformContaining(props, pads.getTile(2, 2));
            const auto& right = platformContaining(props, pads.getTile(4, 2));
            assert(&left != &right);
            assert(left.tiles.size() == 4 && right.tiles.size() == 4);
            for (unsigned int x : {2u, 3u}) {
                assert(&platformContaining(props, pads.getTile(x, 2)) == &left);
                assert(&platformContaining(props, pads.getTile(x, 3)) == &left);
                assert(!pads.getTile(x, 3)->getCollidable());
            }
            for (unsigned int x : {4u, 5u}) {
                assert(&platformContaining(props, pads.getTile(x, 2)) == &right);
                assert(&platformContaining(props, pads.getTile(x, 3)) == &right);
                assert(!pads.getTile(x, 3)->getCollidable());
            }

            vector2 leftMin, leftMax, rightMin, rightMax;
            assert(Kinematics2D::tryGetActiveBounds(pads.getTile(3, 2)->getCollidable(), leftMin, leftMax));
            assert(Kinematics2D::tryGetActiveBounds(pads.getTile(4, 2)->getCollidable(), rightMin, rightMax));
            const float surfaceY = leftMin.y;
            const float seamX = (leftMax.x + rightMin.x) * 0.5f;
            assert(near(rightMin.y, surfaceY));

            // A pad moves as a whole, including its decorative lower row. The pad
            // touching it stays at the surface when the player stands on the left.
            standAt(hero, 48.0f, surfaceY);
            props.update(&hero, false, 0.2f);
            assert(left.occupied && !right.occupied);
            assertPlatformDepth(left, 1.4f);
            assertPlatformDepth(right, 0.0f);

            // The nearest feet surface wins even with more horizontal overlap on
            // the other pad, which is still within the standing tolerance.
            standAt(hero, seamX + 2.0f, surfaceY + left.depth);
            props.update(&hero, false, 0.1f);
            assert(left.occupied && !right.occupied);
            assertPlatformDepth(left, 2.1f);
            assertPlatformDepth(right, 0.0f);

            // Stepping onto the right transfers the weight and lets the left rise.
            standAt(hero, 80.0f, surfaceY);
            props.update(&hero, false, 0.1f);
            assert(!left.occupied && right.occupied);
            assertPlatformDepth(left, 0.1f);
            assertPlatformDepth(right, 0.7f);
            props.update(nullptr, false, 0.1f);
            assert(!left.occupied && !right.occupied);
            assertPlatformDepth(left, 0.0f);
            assertPlatformDepth(right, 0.0f);

            for (float side : {-1.0f, 1.0f}) {
                props.resetPlatforms();
                standAt(hero, seamX + side * 2.0f, surfaceY);
                vector2 bodyMin, bodyMax;
                bodyBounds(hero, bodyMin, bodyMax);
                // Both surfaces overlap the real player body near the seam.
                assert(bodyMin.x < leftMax.x && bodyMax.x > rightMin.x);
                props.update(&hero, false, 0.1f);
                assert(left.occupied == (side < 0.0f));
                assert(right.occupied == (side > 0.0f));
                assertPlatformDepth(left, side < 0.0f ? 0.7f : 0.0f);
                assertPlatformDepth(right, side > 0.0f ? 0.7f : 0.0f);

                // Equal feet distance and equal overlap keep the previous pad.
                props.resetPlatforms();
                standAt(hero, seamX + side * 2.0f, surfaceY);
                props.update(&hero, false, 0.0f);
                standAt(hero, seamX, surfaceY);
                props.update(&hero, false, 0.1f);
                assert(left.occupied == (side < 0.0f));
                assert(right.occupied == (side > 0.0f));
                assertPlatformDepth(left, side < 0.0f ? 0.7f : 0.0f);
                assertPlatformDepth(right, side > 0.0f ? 0.7f : 0.0f);
            }

            // Jumping off releases the pad even while the feet are at its surface.
            props.resetPlatforms();
            standAt(hero, 48.0f, surfaceY);
            props.update(&hero, false, 0.2f);
            standAt(hero, 48.0f, surfaceY + left.depth);
            hero.setVelocity(vector2(0.0f, -20.0f));
            props.update(&hero, false, 0.01f);
            assert(!left.occupied && !right.occupied);
            assertPlatformDepth(left, 1.2f);
            assertPlatformDepth(right, 0.0f);
            props.update(nullptr, false, 0.1f);
            assertPlatformDepth(left, 0.0f);
            assertPlatformDepth(right, 0.0f);
            hero.setVelocity(vector2(0.0f, 0.0f));

            // Respawning must clear occupancy even before the first sink step.
            standAt(hero, 48.0f, surfaceY);
            props.update(&hero, false, 0.0f);
            assert(left.occupied && near(left.depth, 0.0f));
            props.resetPlatforms();
            assert(!left.occupied && !right.occupied);
        }
    }
}

} // namespace

int main()
{
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);
    Engine2D::getEventSystem()->initialize(INFINITE);

    TileSet* tiles = TileSet::loadFromFile("bin/fantasySideScroller/Assets/fantasyTiles.tsj");
    assert(tiles);

    // Neither half of either cattail may create a physics shape, even if an artist
    // places it on a solid layer. The pond plants must never become footholds.
    TileLayerConfig cattailConfig;
    cattailConfig.name = "cattails on terrain";
    cattailConfig.collisionMode = TileCollisionMode::Solid;
    TileMap cattails(2, 2, tiles, cattailConfig);
    cattails.setTile(0, 0, tiles, 516);
    cattails.setTile(0, 1, tiles, 541);
    cattails.setTile(1, 0, tiles, 466);
    cattails.setTile(1, 1, tiles, 491);
    cattails.arrangeTiles();
    assert(!cattails.getTile(0, 0)->getCollidable());
    assert(!cattails.getTile(0, 1)->getCollidable());
    assert(!cattails.getTile(1, 0)->getCollidable());
    assert(!cattails.getTile(1, 1)->getCollidable());

    // Props layer: a key (516) and a closed chest (444-445 over 469-470), far apart.
    TileLayerConfig propsConfig;
    propsConfig.name = "props";
    propsConfig.collisionMode = TileCollisionMode::None;
    TileMap props(20, 4, tiles, propsConfig);
    props.setTile(1, 1, tiles, 515);
    props.setTile(10, 1, tiles, 443);
    props.setTile(11, 1, tiles, 444);
    props.setTile(10, 2, tiles, 468);
    props.setTile(11, 2, tiles, 469);
    props.setTile(4, 2, tiles, 496);
    props.setTile(5, 2, tiles, 521);
    props.setTile(7, 2, tiles, 498);
    props.setTile(8, 2, tiles, 523);
    props.setPosition(vector2(0.0f, 0.0f));
    props.arrangeTiles();

    // One-way layer: a lily pad (493-494) at row 10.
    TileLayerConfig padConfig;
    padConfig.name = "pads";
    padConfig.collisionMode = TileCollisionMode::OneWay;
    TileMap pads(20, 12, tiles, padConfig);
    pads.setTile(15, 10, tiles, 492);
    pads.setTile(16, 10, tiles, 493);
    pads.setTile(15, 11, tiles, 517);
    pads.setTile(16, 11, tiles, 518);
    pads.setPosition(vector2(0.0f, 0.0f));
    pads.arrangeTiles();

    LevelProps levelProps;
    levelProps.initialize({ &props, &pads });
    assert(levelProps.getChests().size() == 1);
    assert(levelProps.getPots().size() == 4);
    assert(levelProps.getChests()[0].tiles.size() == 4);
    assert(near(levelProps.getChests()[0].heal, 30.0f));
    assert(levelProps.getPlatforms().size() == 1);
    assert(levelProps.getPlatforms()[0].tiles.size() == 4);
    assert(!pads.getTile(15, 11)->getCollidable());
    assert(!pads.getTile(16, 11)->getCollidable());
    assert(near(levelProps.getPlatforms()[0].sinkSpeed, 7.0f));

    Character hero;
    hero.setState("Idle");
    hero.addHealth(-50.0f);
    const float woundedHealth = hero.getHealth();

    // Without a key the chest stays shut.
    centreBodyOn(hero, (tileCentre(props.getTile(10, 1)) + tileCentre(props.getTile(11, 2))) * 0.5f);
    levelProps.update(&hero, true, 0.016f);
    assert(levelProps.getLastEvent() == "chest_locked");
    assert(!levelProps.getChests()[0].opened);
    assert(props.getTile(10, 1)->getTileIndex() == 443);

    // Touching the key collects it and removes it from the map.
    centreBodyOn(hero, tileCentre(props.getTile(1, 1)));
    levelProps.update(&hero, false, 0.016f);
    assert(levelProps.getKeyCount() == 1);
    assert(props.getTile(1, 1)->getTileIndex() < 0);
    assert(!props.getTile(1, 1)->getState()->getRenderable()->isVisible());

    // Standing at the chest does nothing until the player interacts.
    centreBodyOn(hero, (tileCentre(props.getTile(10, 1)) + tileCentre(props.getTile(11, 2))) * 0.5f);
    levelProps.update(&hero, false, 0.016f);
    assert(!levelProps.getChests()[0].opened);
    levelProps.update(&hero, true, 0.016f);
    assert(levelProps.getLastEvent() == "chest_opened");
    assert(levelProps.getChests()[0].opened);
    assert(props.getTile(10, 1)->getTileIndex() == 445);
    assert(props.getTile(11, 1)->getTileIndex() == 446);
    assert(props.getTile(10, 2)->getTileIndex() == 470);
    assert(props.getTile(11, 2)->getTileIndex() == 471);
    assert(near(hero.getHealth(), woundedHealth + 30.0f));
    // The key is held, not spent.
    assert(levelProps.getKeyCount() == 1);

    // Standing on the pad sinks it at sink_speed; leaving lets it rise at rise_speed.
    Tile* pad = pads.getTile(15, 10);
    const vector2 padRest = pad->getPosition();
    vector2 padMin, padMax;
    assert(Kinematics2D::tryGetActiveBounds(pad->getCollidable(), padMin, padMax));
    hero.setVelocity(vector2(0.0f, 0.0f));
    for (int i = 0; i < 60; ++i) {
        standAt(hero, padMax.x, padMin.y + levelProps.getPlatforms()[0].depth);
        levelProps.update(&hero, false, 1.0f / 60.0f);
    }
    assert(levelProps.getPlatforms()[0].occupied);
    assert(near(levelProps.getPlatforms()[0].depth, 7.0f, 0.05f));
    assert(near(pad->getPosition().y, padRest.y + 7.0f, 0.05f));
    assert(near(pads.getTile(16, 10)->getPosition().y, padRest.y + 7.0f, 0.05f));
    assert(near(pads.getTile(15, 11)->getPosition().y, padRest.y + 16.0f + 7.0f, 0.05f));
    // It never sinks past sink_depth.
    for (int i = 0; i < 600; ++i) {
        standAt(hero, padMax.x, padMin.y + levelProps.getPlatforms()[0].depth);
        levelProps.update(&hero, false, 1.0f / 60.0f);
    }
    assert(near(levelProps.getPlatforms()[0].depth, 24.0f));
    centreBodyOn(hero, vector2(0.0f, 0.0f));
    for (int i = 0; i < 30; ++i) {
        levelProps.update(&hero, false, 1.0f / 60.0f);
    }
    assert(!levelProps.getPlatforms()[0].occupied);
    assert(near(levelProps.getPlatforms()[0].depth, 14.0f, 0.05f));
    levelProps.resetPlatforms();
    assert(near(pad->getPosition().y, padRest.y));

    checkAdjacentPads(tiles, hero);

    // Pots stay decorative when touched. Only the forward, active sword stroke
    // cracks them, using the matching size and colour from the source atlas.
    standAt(hero, 4 * 16.0f - 8.0f, 48.0f);
    hero.setState("Idle");
    levelProps.update(&hero, false, 0.016f);
    assert(props.getTile(4, 2)->getTileIndex() == 496);
    hero.setState("Attack01");
    auto* slash = static_cast<Animation*>(hero.getRenderable());
    slash->setScale(vector2(1.0f, 1.0f));
    slash->play();
    levelProps.update(&hero, false, 0.016f);
    assert(props.getTile(4, 2)->getTileIndex() == 496);
    slash->update(0.1f);
    slash->update(0.001f);
    assert(slash->getCurrentFrameIndex() > 0);
    levelProps.update(&hero, false, 0.016f);
    assert(props.getTile(4, 2)->getTileIndex() == 497);
    assert(props.getTile(5, 2)->getTileIndex() == 522);
    assert(props.getTile(7, 2)->getTileIndex() == 498);
    hero.setState("Idle");
    standAt(hero, 9 * 16.0f + 8.0f, 48.0f);
    hero.setState("Attack01");
    slash = static_cast<Animation*>(hero.getRenderable());
    slash->setScale(vector2(-1.0f, 1.0f));
    slash->play();
    slash->update(0.1f);
    slash->update(0.001f);
    levelProps.update(&hero, false, 0.016f);
    assert(props.getTile(7, 2)->getTileIndex() == 499);
    assert(props.getTile(8, 2)->getTileIndex() == 524);

    // Knockback launches the character airborne and keeps its push while locked.
    hero.setState("Idle");
    hero.setVelocity(vector2(0.0f, 0.0f));
    hero.applyKnockback(vector2(190.0f, -104.0f), 0.3f);
    assert(!strcmp(hero.getState()->getName(), "Falling"));
    assert(hero.isKnockedBack());
    assert(near(hero.getVelocity().x, 190.0f) && near(hero.getVelocity().y, -104.0f));

    // A hazard pointing right throws the character right (and up), and deals damage.
    hero.resetForRespawn();
    hero.setState("Idle");
    LevelTriggerDescriptor spikes;
    spikes.typeName = "hazard";
    spikes.position = vector2(500.0f, 500.0f);
    spikes.size = vector2(20.0f, 6.0f);
    spikes.properties = { {"damage", "float", "10"}, {"push", "string", "right"}, {"push_speed", "float", "190"} };
    TraversalMechanics mechanics;
    mechanics.initialize({ spikes }, vector2(0.0f, 0.0f));
    centreBodyOn(hero, vector2(510.0f, 503.0f));
    mechanics.update(&hero, 0.016f);
    assert(near(hero.getHealth(), hero.getMaxHealth() - 10.0f));
    assert(near(hero.getVelocity().x, 190.0f));
    assert(hero.getVelocity().y < 0.0f);
    assert(hero.isKnockedBack());

    // An "away" hazard throws the character back toward the side it came from.
    LevelTriggerDescriptor vine;
    vine.typeName = "hazard";
    vine.position = vector2(700.0f, 500.0f);
    vine.size = vector2(12.0f, 80.0f);
    vine.properties = { {"damage", "float", "10"}, {"push", "string", "away"}, {"push_speed", "float", "190"} };
    for (float side : { -1.0f, 1.0f }) {
        hero.resetForRespawn();
        hero.setState("Idle");
        TraversalMechanics thorns;
        thorns.initialize({ vine }, vector2(0.0f, 0.0f));
        centreBodyOn(hero, vector2(706.0f + side * 8.0f, 540.0f));
        thorns.update(&hero, 0.016f);
        assert(near(hero.getHealth(), hero.getMaxHealth() - 10.0f));
        assert(hero.getVelocity().x * side > 100.0f);   // pushed further out on the same side
        assert(hero.getVelocity().y < 0.0f);
    }

    // Between damage ticks the hazard still pushes (it must work as a barrier), without damage.
    {
        hero.resetForRespawn();
        hero.setState("Idle");
        TraversalMechanics thorns;
        thorns.initialize({ vine }, vector2(0.0f, 0.0f));
        centreBodyOn(hero, vector2(698.0f, 540.0f));
        thorns.update(&hero, 0.016f);
        assert(hero.isKnockedBack());
        hero.resetForRespawn();                  // knockback over, full health; hazard cooldown still running
        hero.setState("Idle");
        assert(!hero.isKnockedBack());
        centreBodyOn(hero, vector2(698.0f, 540.0f));
        thorns.update(&hero, 0.016f);
        assert(near(hero.getHealth(), hero.getMaxHealth()));   // still inside the damage interval
        assert(hero.getVelocity().x < -100.0f);                // but thrown back again
    }

    // A lethal hazard hit kills the character instead of throwing it.
    hero.resetForRespawn();
    hero.setState("Idle");
    hero.addHealth(-(hero.getMaxHealth() - 5.0f));
    TraversalMechanics lethal;
    lethal.initialize({ spikes }, vector2(0.0f, 0.0f));
    centreBodyOn(hero, vector2(510.0f, 503.0f));
    lethal.update(&hero, 0.016f);
    assert(hero.getHealth() <= 0.0f);
    assert(!strcmp(hero.getState()->getName(), "Dead"));
    assert(!hero.isKnockedBack());

    // Killzones keep respawning the player every time, not just the first.
    LevelTriggerDescriptor water;
    water.typeName = "killzone";
    water.position = vector2(800.0f, 800.0f);
    water.size = vector2(64.0f, 32.0f);
    TraversalMechanics pond;
    pond.initialize({ water }, vector2(10.0f, 20.0f));
    for (int fall = 0; fall < 3; ++fall) {
        hero.resetForRespawn();
        hero.setState("Idle");
        centreBodyOn(hero, vector2(830.0f, 810.0f));
        pond.update(&hero, 0.016f);
        vector2 respawnAt;
        std::string reason;
        assert(pond.consumeRespawnRequest(respawnAt, &reason));
        assert(reason == "killzone" && near(respawnAt.x, 10.0f) && near(respawnAt.y, 20.0f));
    }

    std::cout << "level props test passed\n";
    return 0;
}
