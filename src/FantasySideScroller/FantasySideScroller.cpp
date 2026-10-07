// src/FantasySideScroller/FantasySideScroller.cpp
#include "FantasySideScroller.h"

FantasySideScroller game;

#include "Resources.h"
#include "Constants.h"
#include "Character.h"
#include "PlayState.h"

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
