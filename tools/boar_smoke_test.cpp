// Headless behavior checks using real sprite-sheet dimensions and game objects.
#include "../src/FantasySideScroller/Boar.h"
#include "../src/FantasySideScroller/Character.h"
#include "../src/Animation.h"
#include "../src/ObjectManager.h"
#include "../src/Tile.h"
#include "../src/FantasySideScroller/LevelManager.h"
#include "../src/GameState.h"
#include "stb/stb_image.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

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
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);
    Engine2D::getEventSystem()->initialize(INFINITE);
    ObjectManager world;
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
    for (int i = 0; i < 1200; ++i) {
        boar.update(1.0f / 60);
        assert(std::fabs(boar.getPosition().x - 100) < 73);
        assert(std::fabs(boar.getPosition().y - 86) < 0.01f);
    }
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
    {
        ObjectManager mapWorld;
        GameState state;
        LevelManager level;
        level.initialize("mosswood_hollow.tmj", vector2(-60, 0), "Background/Background.png", mapWorld, state);
        assert(level.hasSpawnPoint());
        hero.setPosition(level.getSpawnPoint());
        Boar mapBoar(mapWorld, hero, level.getSpawnPoint() + vector2(140, 10));
        const vector2 spawn = mapBoar.getPosition();
        assert(spawn.y < level.getSpawnPoint().y + 100);
        CollisionSystem collision;
        mapWorld.addObject("Boar", &mapBoar);
        for (int i = 0; i < 300; ++i) {
            mapBoar.update(1.0f / 60);
            collision.update(mapWorld.getObjects(), 1.0f / 60);
            assert(mapBoar.getPosition().y < spawn.y + 30);
        }
        std::cout << "Map spawn: " << spawn.x << ", " << spawn.y << "\n";
        collision.reset();
        mapWorld.removeObject(&mapBoar);
        level.shutdown(mapWorld, state);
    }
    std::cout << "PASS: real-map terrain, assets, spawn, patrol, charge, cooldown, directional attack, defeat, reset, ledge avoidance\n";
}
