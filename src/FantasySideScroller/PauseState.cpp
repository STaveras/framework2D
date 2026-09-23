// File: PauseState.cpp
// A pause state that sits on top of PlayState so the game freezes while the
// world stays visible. Pushed when Esc is pressed; popping it resumes play.

#include "PauseState.h"
#include "../Engine2D.h"
#include "Constants.h"
#include "Resources.h"

PauseState::PauseState()
    : _menuRenderList(NULL)
    , _pauseText(NULL)
    , _hintText(NULL)
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

    // Create "PAUSE" text - centered at top
    if (!_pauseText) {
        _pauseText = new Font();
        const std::string fontPath = BasePath("Font/monogram/bitmap/monogram-bitmap.json");
        if (_pauseText->loadFromJSON(fontPath)) {
            _pauseText->setText("PAUSE");
            _pauseText->setTint(0xFFFFFFFF);
            _pauseText->setScale(1.5f, 1.5f);  // Larger for pause text

            // Center horizontally: use getBitmapWidth() as character advance (per RendererGL line 329)
            const int charAdvance = _pauseText->getBitmapWidth();
            const int textWidth = charAdvance * 5;  // 5 characters in "PAUSE"
            const float centerX = renderer->getWidth() / 2.0f;
            const float topY = 80.0f;
            const vector2 pausePos(centerX - textWidth / 2.0f, topY);
            _pauseText->setPosition(pausePos);
            _pauseText->setVisibility(true);
            _menuRenderList->push_back(_pauseText);
        } else {
            DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
            SAFE_DELETE(_pauseText);
        }
    }

    // Create "Press ESC to Resume" hint text - centered below PAUSE
    if (!_hintText) {
        _hintText = new Font();
        const std::string fontPath = BasePath("Font/monogram/bitmap/monogram-bitmap.json");
        if (_hintText->loadFromJSON(fontPath)) {
            _hintText->setText("Press ESC to Resume");
            _hintText->setTint(0xFFAAAAAA);
            _hintText->setScale(0.8f, 0.8f);  // Smaller for hint text

            // Center horizontally: use getBitmapWidth() as character advance (per RendererGL line 329)
            const int charAdvance = _hintText->getBitmapWidth();
            const int textWidth = charAdvance * 21;  // 21 characters in "Press ESC to Resume"
            const float centerX = renderer->getWidth() / 2.0f;
            const float hintY = 140.0f;
            const vector2 hintTextPos(centerX - textWidth / 2.0f, hintY);
            _hintText->setPosition(hintTextPos);
            _hintText->setVisibility(true);
            _menuRenderList->push_back(_hintText);
        } else {
            DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
            SAFE_DELETE(_hintText);
        }
    }
}

bool PauseState::onExecute(float time)
{
    Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

    // ESC to resume (pop this state)
    if (keyboard->keyPressed(keyboard->getKeys().KBK_ESCAPE)) {
        Engine2D::getGame()->pop();
        return false;  // Stop executing this state
    }

    // Q to quit
    if (keyboard->keyPressed(keyboard->getKeys().KBK_Q)) {
        Engine2D::quit();
        return false;
    }

    return true;  // Keep running (state stays on top)
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

    if (_menuRenderList) {
        IRenderer* renderer = Engine2D::getRenderer();
        if (renderer) {
            renderer->destroyRenderList(_menuRenderList);
        }
        _menuRenderList = NULL;
    }
}
