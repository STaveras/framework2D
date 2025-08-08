// src/FantasySideScroller/FantasySideScroller.cpp
#include "FantasySideScroller.h"

FantasySideScroller game;

#include "Resources.h"
#include "Constants.h"
#include "Character.h"
#include "PlayState.h"

void FantasySideScroller::begin()
{
    Game::begin();

    // configure window & renderer
    Renderer::mainWindow->setWidth(GAME_RES_X * WINDOW_SIZE_MULTIPLIER);
    Renderer::mainWindow->setHeight(GAME_RES_Y * WINDOW_SIZE_MULTIPLIER);

    Renderer::get()->setWidth(GAME_RES_X);
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
