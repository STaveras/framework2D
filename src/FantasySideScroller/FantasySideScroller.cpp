// src/FantasySideScroller/FantasySideScroller.cpp
#include "FantasySideScroller.h"

FantasySideScroller game;

#include "Resources.h"
#include "Constants.h"
#include "Character.h"
#include "PlayState.h"
#include "../InputMap.h"

#include <algorithm>
#include <cmath>

void FantasySideScroller::begin()
{
    Game::begin();

    // configure window & renderer
    int width = GAME_RES_X;
#if FRAMEWORK_IOS
    // The screen sets the size here, not the game: keep the game's height and
    // widen the view to the screen's shape (narrower screens get letterboxed).
    const int clientWidth = Renderer::mainWindow->getClientWidth();
    const int clientHeight = Renderer::mainWindow->getClientHeight();
    if (clientWidth > 0 && clientHeight > 0) {
        width = std::max(GAME_RES_X, (int)std::lround((double)GAME_RES_Y * clientWidth / clientHeight));
    }
#else
    Renderer::mainWindow->setWidth(GAME_RES_X * WINDOW_SIZE_MULTIPLIER);
    Renderer::mainWindow->setHeight(GAME_RES_Y * WINDOW_SIZE_MULTIPLIER);
#endif

    Renderer::get()->setWidth(width);
    Renderer::get()->setHeight(GAME_RES_Y);

    Renderer::get()->shutdown();
    Renderer::mainWindow->resize();
    Renderer::get()->initialize();

    // push our play state once
    if (!_playState) {
        _playState = new PlayState();

        // Do some preloading here

        this->push(_playState);
    }
}

void FantasySideScroller::end()
{
    Game::end();

    if (_playState) {
        delete _playState;
        _playState = nullptr;
    }
}

void FantasySideScroller::bindInputs(InputMap* inputMap) const
{
    if (!inputMap) {
        return;
    }

    Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

    // TODO: Save the keymappings to a file and load them here
    inputMap->addAction(Action("JUMP", keyboard->getKeys().KBK_SPACE));
    inputMap->addAction(Action("JUMP", Gamepad::Button::A));
    inputMap->addAction(Action("LEFT", keyboard->getKeys().KBK_LEFT));
    inputMap->addAction(Action("LEFT", keyboard->getKeys().KBK_A));
    inputMap->addAction(Action("LEFT", Gamepad::Button::DpadLeft));
    Action leftStick("LEFT");
    leftStick.assignAxis(Gamepad::Axis::LeftX, -0.25f);
    inputMap->addAction(leftStick);
    inputMap->addAction(Action("RIGHT", keyboard->getKeys().KBK_RIGHT));
    inputMap->addAction(Action("RIGHT", keyboard->getKeys().KBK_D));
    inputMap->addAction(Action("RIGHT", Gamepad::Button::DpadRight));
    Action rightStick("RIGHT");
    rightStick.assignAxis(Gamepad::Axis::LeftX, 0.25f);
    inputMap->addAction(rightStick);
    inputMap->addAction(Action("DOWN", keyboard->getKeys().KBK_DOWN));
    inputMap->addAction(Action("DOWN", keyboard->getKeys().KBK_S));
    inputMap->addAction(Action("DOWN", Gamepad::Button::DpadDown));
    inputMap->addAction(Action("ATTACK", keyboard->getKeys().KBK_LCONTROL));
    inputMap->addAction(Action("ATTACK", keyboard->getKeys().KBK_Z));
    inputMap->addAction(Action("ATTACK", Gamepad::Button::X));
    inputMap->addAction(Action("RUN", keyboard->getKeys().KBK_LSHIFT));
    inputMap->addAction(Action("RUN", Gamepad::Button::LeftBumper));
    // "UP" is the semantic up-direction action, shared with the pause menu.
    // "INTERACT" is gameplay-only (E key, or the left stick button, which is
    // the iOS touch controls' USE) for interacting with objects.
    inputMap->addAction(Action("UP", keyboard->getKeys().KBK_UP));
    inputMap->addAction(Action("UP", keyboard->getKeys().KBK_W));
    inputMap->addAction(Action("UP", Gamepad::Button::DpadUp));
    Action upStick("UP");
    upStick.assignAxis(Gamepad::Axis::LeftY, -0.5f);
    inputMap->addAction(upStick);
    inputMap->addAction(Action("INTERACT", keyboard->getKeys().KBK_E));
    inputMap->addAction(Action("INTERACT", Gamepad::Button::LeftThumb));
    // "CONFIRM" is the menu confirm action: A + Enter.
    inputMap->addAction(Action("CONFIRM", Gamepad::Button::A));
    inputMap->addAction(Action("CONFIRM", keyboard->getKeys().KBK_RETURN));
    inputMap->addAction(Action("PAUSE", keyboard->getKeys().KBK_ESCAPE));
    inputMap->addAction(Action("PAUSE", Gamepad::Button::Start));
}
