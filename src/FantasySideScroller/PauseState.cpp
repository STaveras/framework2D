// File: PauseState.cpp
// A pause state that sits on top of PlayState so the game freezes while the
// world stays visible. Pushed when PAUSE is pressed; pressing it again resumes.

#include "PauseState.h"
#include "../Cursor.h"
#include "../Engine2D.h"
#include "../IMouse.h"
#include "Constants.h"
#include "Resources.h"
#include "ScreenSpaceCursor.h"

PauseState::PauseState()
    : _menuRenderList(NULL)
    , _pauseText(NULL)
    , _hintText(NULL)
    , _menu(NULL)
    , _cursor(NULL)
    , _inputMap(NULL)
{
}

PauseState::~PauseState()
{
    if (_pauseText) {
        SAFE_DELETE(_pauseText);
    }
    if (_hintText) {
        SAFE_DELETE(_hintText);
    }
    SAFE_DELETE(_menu);
    SAFE_DELETE(_cursor);
    if (_menuRenderList) {
        IRenderer* renderer = Engine2D::getRenderer();
        if (renderer) {
            renderer->destroyRenderList(_menuRenderList);
        }
        _menuRenderList = NULL;
    }
}

void PauseState::onEnter(State* prev)
{
    // We don't call GameState::onEnter because we don't need its object/collision/input managers
    // We only need a screen-space render list for the pause menu

    IRenderer* renderer = Engine2D::getRenderer();
    if (!renderer) {
        return;
    }

    if (!_menuRenderList) {
        _menuRenderList = renderer->createRenderList(true);  // true = screenSpace
    }

    const std::string fontPath = BasePath("Font/monogram/bitmap/monogram-bitmap.json");
    const float centerX = renderer->getWidth() / 2.0f;

    // Create "PAUSE" text - centered near the top
    if (!_pauseText) {
        _pauseText = new Font();
        if (_pauseText->loadFromJSON(fontPath)) {
            _pauseText->setText("PAUSE");
            _pauseText->setTint(0xFFFFFFFF);
            _pauseText->setScale(1.5f, 1.5f);  // Larger for pause text

            const float textWidth = _pauseText->getTextWidth() * _pauseText->getScale().x;
            const float centerY = 40.0f;
            const float textHeight = _pauseText->getHeight() * _pauseText->getScale().y;
            const vector2 pausePos(centerX - textWidth / 2.0f, centerY - textHeight / 2.0f);
            _pauseText->setPosition(pausePos);
            _pauseText->setVisibility(true);
            _menuRenderList->push_back(_pauseText);
        } else {
            DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
            SAFE_DELETE(_pauseText);
        }
    }

    // Create the RESUME / QUIT menu (its own screen-space render list, so it
    // draws in front of the PAUSE title and hint)
    _createMenu(renderer);

    // Create hint text - centered below the menu options
    if (!_hintText) {
        _hintText = new Font();
        if (_hintText->loadFromJSON(fontPath)) {
#if FRAMEWORK_IOS
            // The touch controls' pause button, or a controller's menu button
            _hintText->setText("|| / MENU to Resume");
#else
            _hintText->setText("ESC / OPTIONS to Resume");
#endif
            _hintText->setTint(0xFFAAAAAA);
            _hintText->setScale(0.8f, 0.8f);  // Smaller for hint text

            const float textWidth = _hintText->getTextWidth() * _hintText->getScale().x;
            const float centerY = 160.0f;
            const float textHeight = _hintText->getHeight() * _hintText->getScale().y;
            const vector2 hintTextPos(centerX - textWidth / 2.0f, centerY - textHeight / 2.0f);
            _hintText->setPosition(hintTextPos);
            _hintText->setVisibility(true);
            _menuRenderList->push_back(_hintText);
        } else {
            DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
            SAFE_DELETE(_hintText);
        }
    }

    // Create the mouse cursor so it stays visible while the game is paused.
    // It lives in this screen-space render list (created after the game's HUD
    // list) so it draws on top of the menu text.
    // No cursor without a pointer (touch-only iOS has no mouse).
    IInput* input = Engine2D::getInput();
    if (!_cursor && (!input || input->getMouse())) {
        _cursor = new Cursor();
        if (_cursor->load(BasePath("cursors.png").c_str())) {
            _menuRenderList->push_back(_cursor->getImage());
        } else {
            SAFE_DELETE(_cursor);
        }
    }
}

bool PauseState::onExecute(float time)
{
    Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

    // Use the same action as gameplay for either Escape or controller Options.
    Action* pauseAction = _inputMap ? _inputMap->getAction("PAUSE") : NULL;
    const bool pausePressed = pauseAction
        ? _inputMap->buttonPressed(pauseAction)
        : keyboard->keyPressed(keyboard->getKeys().KBK_ESCAPE);
    if (pausePressed) {
        Engine2D::getGame()->pop();
        return false;  // Stop executing this state
    }

    // Move between the menu options using the semantic direction actions:
    // UP moves up (previous), DOWN moves down (next). These are the same
    // actions used for gameplay movement, so no redundant binds.
    if (_menu && _inputMap) {
        Action* upAction = _inputMap->getAction("UP");
        Action* downAction = _inputMap->getAction("DOWN");
        if (upAction && _inputMap->buttonPressed(upAction)) {
            _menu->selectPrevious();
        } else if (downAction && _inputMap->buttonPressed(downAction)) {
            _menu->selectNext();
        }
    }

    // Execute the selected option with the CONFIRM action (Space / A / Enter).
    if (_inputMap) {
        Action* confirmAction = _inputMap->getAction("CONFIRM");
        if (confirmAction && _inputMap->buttonPressed(confirmAction)) {
            _executeSelectedOption();
            return false;
        }
    }

    // Keep the menu cursor tracking the mouse while paused.
    if (_cursor && _menu) {
        Mouse* mouse = Engine2D::getInput()->getMouse();
        if (mouse) {
            // Hovering an option with the mouse selects it; clicking executes it.
            const vector2 renderPos = ClientToRenderCursorPosition(mouse->getPosition());
            const int hoverIndex = _menu->hitTest(renderPos);
            if (hoverIndex >= 0) {
                _menu->setSelection(hoverIndex);
                if (mouse->buttonPressed(MOUSE_LEFT)) {
                    _executeSelectedOption();
                    return false;
                }
            }

            _cursor->setPosition(renderPos);
            _cursor->updateFromMouse(mouse);
        }
    }

    // Highlight the currently selected option
    if (_menu) {
        _menu->applyTint(0xFFFFFFFF, 0xFF888888);
    }

    return true;  // Keep running (state stays on top)
}

void PauseState::_createMenu(IRenderer* renderer)
{
    if (_menu) {
        return;
    }

    // The menu container stacks its items (RESUME, then QUIT directly below
    // it) starting just under the PAUSE title.
    const vector2 menuTop(renderer->getWidth() / 2.0f, 72.0f);
    _menu = Widgets::createMenu(
        renderer, BasePath("Font/monogram/bitmap/monogram-bitmap.json"), menuTop);
    if (_menu) {
        _menu->addItem("RESUME");
        _menu->addItem("QUIT");
    }
}

void PauseState::_executeSelectedOption(void)
{
    if (!_menu) {
        return;
    }
    switch (_menu->getSelection()) {
        case OPTION_QUIT:
            Engine2D::quit();
            break;
        default: // OPTION_RESUME and anything else
            // Resume play by popping this state
            Engine2D::getGame()->pop();
            break;
    }
}

void PauseState::onExit(State* next)
{
    // Remove text from render list before destroying it
    if (_pauseText) {
        _menuRenderList->remove(_pauseText);
        SAFE_DELETE(_pauseText);
        _pauseText = NULL;
    }
    if (_hintText) {
        _menuRenderList->remove(_hintText);
        SAFE_DELETE(_hintText);
        _hintText = NULL;
    }
    SAFE_DELETE(_menu);
    if (_cursor && _cursor->getImage()) {
        _menuRenderList->remove(_cursor->getImage());
    }
    SAFE_DELETE(_cursor);

    if (_menuRenderList) {
        IRenderer* renderer = Engine2D::getRenderer();
        if (renderer) {
            renderer->destroyRenderList(_menuRenderList);
        }
        _menuRenderList = NULL;
    }
}
