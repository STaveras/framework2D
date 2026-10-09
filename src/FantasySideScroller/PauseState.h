// File: PauseState.h
// A pause state that sits on top of PlayState so the game freezes while the
// world stays visible. Pushed when PAUSE is pressed; pressing it again resumes.

#pragma once

#include "../GameState.h"
#include "../Font.h"
#include "../Widgets.h"

class Cursor;
class InputMap;

class PauseState : public GameState
{
    IRenderer::RenderList* _menuRenderList = NULL;
    Font* _pauseText = NULL;
    Font* _hintText = NULL;
    Menu* _menu = NULL;
    Cursor* _cursor = NULL;
    InputMap* _inputMap = NULL;

    enum Option { OPTION_RESUME = 0, OPTION_QUIT = 1 };

    void _createMenu(IRenderer* renderer);
    void _executeSelectedOption(void);

public:
    PauseState(void);
    virtual ~PauseState(void);

    void setInputMap(InputMap* controller) { _inputMap = controller; }

    void onEnter(State* prev) override;
    bool onExecute(float time) override;
    void onExit(State* next) override;
};
