// File: GameOverState.h
// A game-over state that sits on top of PlayState so the world freezes (with
// the dead character visible) while the GAME OVER prompt is shown.  Pushed
// when the playable character dies; popping it resumes play.
//
// The prompt is a "RETRY? Y/N" question, exposed through the same Menu widget
// the pause screen uses: RETRY (Y) respawns and returns to play, QUIT (N) ends
// the game.  The menu reads the player's InputMap (like the pause menu), so
// the D-pad / stick, A, and the iOS touch controls work as well as the keys.

#pragma once

#include "../GameState.h"
#include "../Font.h"
#include "../Widgets.h"

class Cursor;

class GameOverState : public GameState
{
    IRenderer::RenderList* _menuRenderList = NULL;
    Font* _titleText = NULL;
    Font* _hintText = NULL;
    Menu* _menu = NULL;
    Cursor* _cursor = NULL;
    bool _retryRequested = false;

    enum Option { OPTION_RETRY = 0, OPTION_QUIT = 1 };

    void _createMenu(IRenderer* renderer);
    void _executeSelectedOption(void);

public:
    GameOverState(void);
    virtual ~GameOverState(void);

    void onEnter(State* prev) override;
    bool onExecute(float time) override;
    void onExit(State* next) override;

    // True if the player chose to retry (consumed on next read).
    bool wasRetryRequested(void) const { return _retryRequested; }
    void consumeRetryRequest(void) { _retryRequested = false; }
};
