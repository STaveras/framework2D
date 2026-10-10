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

FantasySideScroller::FantasySideScroller(void) = default;
FantasySideScroller::~FantasySideScroller(void) = default;

void FantasySideScroller::begin()
{
    Game::begin();

    if (!_inputMap) {
        _inputMap = &_inputManager.createInputMap();
        _bindInputs(*_inputMap);
    }

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
        _playState = std::make_unique<PlayState>();

        // Do some preloading here

        this->push(_playState.get());
    }
}

void FantasySideScroller::end()
{
    Game::end();
    _inputMap = nullptr; // Game::end() released the maps
    _playState.reset();
}

void FantasySideScroller::_bindInputs(InputMap& input)
{
    using Axis = Gamepad::Axis;
    using Button = Gamepad::Button;

    // TODO: Save the keymappings to a file and load them here
    input.bind("JUMP").key(Key::Space).button(Button::A);
    input.bind("LEFT").key(Key::Left).key(Key::A).button(Button::DpadLeft).axis(Axis::LeftX, -0.25f);
    input.bind("RIGHT").key(Key::Right).key(Key::D).button(Button::DpadRight).axis(Axis::LeftX, 0.25f);
    input.bind("DOWN").key(Key::Down).key(Key::S).button(Button::DpadDown);
    input.bind("ATTACK").key(Key::LeftControl).key(Key::Z).button(Button::X);
    input.bind("RUN").key(Key::LeftShift).button(Button::LeftBumper);
    // "UP" is the semantic up-direction action, shared with the pause menu.
    input.bind("UP").key(Key::Up).key(Key::W).button(Button::DpadUp).axis(Axis::LeftY, -0.5f);
    // "INTERACT" is gameplay-only (E key, or the left stick button, which is
    // the iOS touch controls' USE) for interacting with objects.
    input.bind("INTERACT").key(Key::E).button(Button::LeftThumb);
    // "CONFIRM" is the menu confirm action: A + Enter.
    input.bind("CONFIRM").button(Button::A).key(Key::Enter);
    input.bind("PAUSE").key(Key::Escape).button(Button::Start);
}
