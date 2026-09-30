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

} // namespace

int main()
{
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);
    Engine2D::getEventSystem()->initialize(INFINITE);

    TileSet* tiles = TileSet::loadFromFile("bin/fantasySideScroller/Assets/fantasyTiles.tsj");
    assert(tiles);

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
    props.setPosition(vector2(0.0f, 0.0f));
    props.arrangeTiles();

    // One-way layer: a lily pad (493-494) at row 10.
    TileLayerConfig padConfig;
    padConfig.name = "pads";
    padConfig.collisionMode = TileCollisionMode::OneWay;
    TileMap pads(20, 12, tiles, padConfig);
    pads.setTile(15, 10, tiles, 492);
    pads.setTile(16, 10, tiles, 493);
    pads.setPosition(vector2(0.0f, 0.0f));
    pads.arrangeTiles();

    LevelProps levelProps;
    levelProps.initialize({ &props, &pads });
    assert(levelProps.getChests().size() == 1);
    assert(levelProps.getChests()[0].tiles.size() == 4);
    assert(near(levelProps.getChests()[0].heal, 30.0f));
    assert(levelProps.getPlatforms().size() == 1);
    assert(levelProps.getPlatforms()[0].tiles.size() == 2);
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
