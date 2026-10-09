// File: GameOverState.cpp
// A game-over state that sits on top of PlayState so the world freezes (with
// the dead character visible) while the GAME OVER prompt is shown.  Pushed
// when the playable character dies; popping it resumes play.

#include "GameOverState.h"

#include "../Cursor.h"
#include "../Engine2D.h"
#include "../IMouse.h"
#include "Constants.h"
#include "Resources.h"
#include "ScreenSpaceCursor.h"

GameOverState::GameOverState()
    : _menuRenderList(NULL)
    , _titleText(NULL)
    , _hintText(NULL)
    , _menu(NULL)
    , _cursor(NULL)
    , _retryRequested(false)
{
}

GameOverState::~GameOverState()
{
    if (_titleText) {
        SAFE_DELETE(_titleText);
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

void GameOverState::onEnter(State* prev)
{
    // We don't call GameState::onEnter because we don't need its object/collision/input
    // managers; we only need a screen-space render list for the game-over prompt.

    IRenderer* renderer = Engine2D::getRenderer();
    if (!renderer) {
        return;
    }

    if (!_menuRenderList) {
        _menuRenderList = renderer->createRenderList(true);  // true = screenSpace
    }

    const std::string fontPath = BasePath("Font/monogram/bitmap/monogram-bitmap.json");
    const float centerX = renderer->getWidth() / 2.0f;

    // Create "GAME OVER" text - centered near the top
    if (!_titleText) {
        _titleText = new Font();
        if (_titleText->loadFromJSON(fontPath)) {
            _titleText->setText("GAME OVER");
            _titleText->setTint(0xFFFF5555);  // red for game over
            _titleText->setScale(1.5f, 1.5f);

            const float textWidth = _titleText->getTextWidth() * _titleText->getScale().x;
            const float centerY = 40.0f;
            const float textHeight = _titleText->getHeight() * _titleText->getScale().y;
            const vector2 titlePos(centerX - textWidth / 2.0f, centerY - textHeight / 2.0f);
            _titleText->setPosition(titlePos);
            _titleText->setVisibility(true);
            _menuRenderList->push_back(_titleText);
        } else {
            DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
            SAFE_DELETE(_titleText);
        }
    }

    // Create the RETRY / QUIT menu (its own screen-space render list, so it
    // draws in front of the GAME OVER title and hint)
    _createMenu(renderer);

    // Create hint text - centered below the menu options
    if (!_hintText) {
        _hintText = new Font();
        if (_hintText->loadFromJSON(fontPath)) {
            _hintText->setText("Y: RETRY   N: QUIT");
            _hintText->setTint(0xFFAAAAAA);
            _hintText->setScale(0.8f, 0.8f);

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

    // Create the mouse cursor so it stays visible while the game is over.
    if (!_cursor) {
        _cursor = new Cursor();
        if (_cursor->load(BasePath("cursors.png").c_str())) {
            _menuRenderList->push_back(_cursor->getImage());
        } else {
            SAFE_DELETE(_cursor);
        }
    }
}

bool GameOverState::onExecute(float time)
{
    (void)time;
    Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

    // Y to retry: flag the request and pop; PlayState respawns on the way back up.
    if (keyboard->keyPressed(keyboard->getKeys().KBK_Y)) {
        _retryRequested = true;
        Engine2D::getGame()->pop();
        return false;  // Stop executing this state
    }

    // N to quit
    if (keyboard->keyPressed(keyboard->getKeys().KBK_N)) {
        Engine2D::quit();
        return false;
    }

    // Arrow keys to move between the menu options
    if (_menu) {
        if (keyboard->keyPressed(keyboard->getKeys().KBK_UP)) {
            _menu->selectPrevious();
        } else if (keyboard->keyPressed(keyboard->getKeys().KBK_DOWN)) {
            _menu->selectNext();
        }
    }

    // Enter or Space executes the selected option
    if (keyboard->keyPressed(keyboard->getKeys().KBK_RETURN) ||
        keyboard->keyPressed(keyboard->getKeys().KBK_SPACE)) {
        _executeSelectedOption();
        return false;
    }

    // Keep the menu cursor tracking the mouse while the game is over.
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

void GameOverState::_createMenu(IRenderer* renderer)
{
    if (_menu) {
        return;
    }

    // The menu container stacks its items (RETRY, then QUIT directly below it)
    // starting just under the GAME OVER title.
    const vector2 menuTop(renderer->getWidth() / 2.0f, 72.0f);
    _menu = Widgets::createMenu(
        renderer, BasePath("Font/monogram/bitmap/monogram-bitmap.json"), menuTop);
    if (_menu) {
        _menu->addItem("RETRY");
        _menu->addItem("QUIT");
    }
}

void GameOverState::_executeSelectedOption(void)
{
    if (!_menu) {
        return;
    }
    switch (_menu->getSelection()) {
        case OPTION_QUIT:
            Engine2D::quit();
            break;
        default: // OPTION_RETRY and anything else
            // Retry: flag the request and pop; PlayState respawns on the way back up.
            _retryRequested = true;
            Engine2D::getGame()->pop();
            break;
    }
}

void GameOverState::onExit(State* next)
{
    (void)next;
    // Remove text from render list before destroying it
    if (_titleText) {
        _menuRenderList->remove(_titleText);
        SAFE_DELETE(_titleText);
        _titleText = NULL;
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
