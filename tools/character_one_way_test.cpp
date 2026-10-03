// Exercise Character support, state transitions, and the real collision solver headlessly.
#include "../src/FantasySideScroller/Character.h"
#include "../src/GameState.h"
#include "../src/InputEvent.h"
#include "../src/Kinematics2D.h"
#include "../src/PlayerController.h"
#include "../src/Square.h"
#include "stb/stb_image.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
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

class TestInput : public IInput {
public:
    void initialize() override {}
    void update() override {}
    void shutdown() override {}
};
class TestGame : public Game {
public:
    void begin() override {}
    void end() override {}
    Player* addPlayer() { return _players.create(); }
};
class TestState : public GameState {
public:
    void onEnter(State*) override {}
    void onExit(State*) override {}
    void step(float dt) {
        _objectManager.update(dt);
        _collisionSystem.update(_objectManager, dt);
    }
    void resetCollision() { _collisionSystem.reset(); }
};
class TestTileSet : public TileSet {
    Square shape{vector2(0, 0), 32, 8};
public:
    explicit TestTileSet(ITexture* texture) : TileSet(texture, 32) {
        _tileInfo[0] = {"tile", &shape};
    }
};

int main() {
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    TestInput hardware;
    Engine2D::setInputInterface(&hardware);
    Engine2D::setRenderer(&renderer);
    Engine2D::getEventSystem()->initialize(INFINITE);
    TestGame game;
    Engine2D::setGame(&game);
    TestState scene;
    game.push(&scene);
    InputMap controller;
    controller.addAction(Action("JUMP"));
    controller.addAction(Action("DOWN"));
    controller.addAction(Action("LEFT"));
    controller.addAction(Action("RIGHT"));
    Character hero;
    Player* player = game.addPlayer();
    player->setInputMap(&controller);
    player->setGameObject(&hero);
    PlayerController heroController(&controller);
    Character::bindPlayerActions(heroController);
    hero.possess(&heroController);
    player->start();
    TestTileSet tiles(renderer.createTexture("bin/fantasySideScroller/Character/Idle/Idle-Sheet.png"));
    Tile platform(0, &tiles), floor(0, &tiles), lower(0, &tiles), adjacent(0, &tiles);
    Square platformShape(vector2(0, 0), 128, 8);
    Square floorShape(vector2(-500, 0), 1000, 16);
    platform.getState()->setCollidable(&platformShape);
    platform.setLayerCollisionMode(TileCollisionMode::OneWay);
    platform.setPosition(0, 100);
    floor.getState()->setCollidable(&floorShape);
    floor.setLayerCollisionMode(TileCollisionMode::Solid);
    floor.setPosition(0, 132);
    lower.getState()->setCollidable(&platformShape);
    lower.setLayerCollisionMode(TileCollisionMode::OneWay);
    lower.setPosition(1000, 116);
    adjacent.getState()->setCollidable(&platformShape);
    adjacent.setLayerCollisionMode(TileCollisionMode::OneWay);
    adjacent.setPosition(1200, 100);
    auto* world = scene.getObjectManager();
    world->addObject("platform", &platform);
    world->addObject("floor", &floor);
    world->addObject("hero", &hero);
    world->addObject("lower", &lower);
    world->addObject("adjacent", &adjacent);
    float dt = 1.0f / 60.0f;
    auto foot = [&]() {
        vector2 lo, hi;
        assert(Kinematics2D::tryGetActiveBounds(hero.getCollidable(), lo, hi));
        return hi.y;
    };
    auto input = [&](const char* action, bool down) {
        auto* a = controller.getAction(action);
        const bool changed = a->isActive() != down;
        a->setActive(down);
        auto* events = Engine2D::getEventSystem();
        events->sendEvent<InputEvent>(InputEvent(down ? EVT_KEYDOWN : EVT_KEYUP,
            &controller, 0, action), nullptr, Event::event_priority_immediate);
        if (changed) events->sendEvent<InputEvent>(InputEvent(down ? EVT_KEYPRESSED : EVT_KEYRELEASED,
            &controller, 0, action), nullptr, Event::event_priority_immediate);
    };
    auto tick = [&]() {
        // Held/up events arrive before object updates, as in GameState::onExecute.
        for (const char* name : {"DOWN", "JUMP", "LEFT", "RIGHT"})
            input(name, controller.getAction(name)->isActive());
        scene.step(dt);
    };
    auto run = [&](float seconds) {
        for (int i = 0; i < static_cast<int>(std::ceil(seconds / dt)); ++i) tick();
    };
    auto place = [&](float x, float footY, const char* state = "Idle") {
        input("JUMP", false);
        input("DOWN", false);
        hero.resetForRespawn();
        hero.setState(state);
        hero.setPosition(x, footY - 24);
        hero.setVelocity(vector2(0, 0));
        scene.resetCollision();
    };
    int failures = 0;
    auto check = [&](bool condition, const std::string& label) {
        if (condition) return;
        ++failures;
        const auto& s = hero.getKinematicState2D();
        std::cerr << "FAIL " << label << " dt=" << dt << " state=" << hero.getState()->getName()
            << " foot=" << foot() << " contacts=" << s.groundContacts.size()
            << " timer=" << s.dropThroughTimer << " pending=" << s.dropThroughResumePending
            << " resume=" << s.dropThroughResumeTopY << std::endl;
    };
    auto atFoot = [&](float y) { return std::fabs(foot() - y) < 0.05f; };
    auto drop = [&]() {
        input("JUMP", false);
        tick();
        input("DOWN", true);
        input("JUMP", true);
        tick();
        check(std::string(hero.getState()->getName()) == "Falling", "drop begins falling");
    };
    auto jump = [&]() {
        input("DOWN", false);
        input("JUMP", false);
        tick();
        input("JUMP", true);
    };

    for (float step : {1.0f / 30.0f, 1.0f / 60.0f, 1.0f / 120.0f}) {
        dt = step;
        for (float gap : {4.0f, 16.0f, 32.0f, 96.0f}) {
            floor.setPosition(0, 100 + gap);
            place(64, 100);
            run(0.1f);
            check(atFoot(100), "initial platform support");
            for (int cycle = 0; cycle < 3; ++cycle) {
                drop();
                run(1.2f);
                check(atFoot(100 + gap), "land on solid floor after drop");
                jump();
                run(2.0f);
                check(atFoot(100), "jump back and land on the same platform");
            }
        }

        // A lower one-way platform must catch the character while the short
        // departure timer is still active; holding the chord must not repeat.
        floor.setPosition(0, 300);
        lower.setPosition(0, 116);
        place(64, 100);
        run(0.1f);
        drop();
        run(0.14f);
        check(atFoot(116), "nearby lower one-way platform catches drop");
        run(0.5f);
        check(atFoot(116), "holding down+jump does not repeat the drop");
        drop();
        run(1.2f);
        check(atFoot(300), "second press drops through the lower platform");
        lower.setPosition(1000, 116);

        floor.setPosition(0, 164);
        // State changes and facing shift the 20px collider relative to the
        // tile; overlap at either edge remains valid support.
        for (float facing : {-1.0f, 1.0f}) {
            for (float x : {1.0f, 64.0f, 127.0f}) {
                place(x, 100);
                hero.getRenderable()->setScale(vector2(facing, 1));
                run(0.1f);
                check(atFoot(100), "edge stays supported before input");
                drop();
                run(1.2f);
                check(atFoot(164), "edge drop reaches floor");
                jump();
                run(2.0f);
                check(atFoot(100), "edge catches ordinary landing");
            }
        }

        // Dropping while crossing a seam must ignore both pieces of the
        // departure surface, even when only one was a previous contact.
        adjacent.setPosition(128, 100);
        place(127, 100);
        run(0.1f);
        input("RIGHT", true);
        drop();
        run(0.15f);
        input("RIGHT", false);
        run(1.0f);
        check(atFoot(164), "moving drop across adjacent platform tiles");
        adjacent.setPosition(1200, 100);

        // DOWN+JUMP on solid ground keeps its normal jump behavior.
        place(250, 164);
        run(0.1f);
        input("DOWN", true);
        input("JUMP", true);
        run(0.2f);
        check(foot() < 164, "solid ground still supports normal jumping");
        run(1.5f);
        check(atFoot(164), "solid ground remains solid after down+jump");

        for (const char* state : {"Landing", "Attack01", "Attack02"}) {
            place(64, 100, state);
            tick();
            drop();
            run(1.2f);
            check(atFoot(164), "drop during grounded animation reaches floor");
        }

        // A low jump that never clears the platform must not be pulled onto
        // its top by the character's separate ground-support scan.
        place(64, 104, "Falling");
        run(0.12f);
        check(foot() > 104, "no ground snap from under a one-way platform");

        // The timer can expire while a thick platform still surrounds the
        // character. Its side/underside must not stop the drop.
        floor.setPosition(0, 300);
        platformShape.setHeight(96);
        platform.refreshCollisionGeometry();
        place(64, 100);
        run(0.1f);
        drop();
        run(1.5f);
        check(atFoot(300), "drop clears a thick one-way body");
        platformShape.setHeight(8);
        platform.refreshCollisionGeometry();
    }

    player->finish();
    game.pop();
    Engine2D::setGame(nullptr);
    std::cout << "Character one-way behavior: " << failures << " failures" << std::endl;
    return failures ? 1 : 0;
}
