// File: MenuState.cpp

#include "MenuState.h"

#include "../Cursor.h"
#include "../Engine2D.h"
#include "../Font.h"
#include "../IInput.h"
#include "../InputMap.h"
#include "../Widgets.h"
#include "FantasySideScroller.h"
#include "Resources.h"

namespace {
constexpr float kTitleCenterY = 40.0f;
constexpr float kTitleScale = 1.5f;
constexpr float kMenuTopY = 72.0f; // the options stack down from just under the title
constexpr float kHintCenterY = 160.0f;
constexpr float kHintScale = 0.8f;
constexpr uint32_t kHintTint = 0xFFAAAAAA;
constexpr uint32_t kSelectedTint = 0xFFFFFFFF;
constexpr uint32_t kUnselectedTint = 0xFF888888;

std::string fontPath(void)
{
	return BasePath("Font/monogram/bitmap/monogram-bitmap.json");
}
}

MenuState::MenuState(std::string title, Color titleTint, std::vector<std::string> options, std::string hint)
	: _title(std::move(title))
	, _titleTint(titleTint)
	, _options(std::move(options))
	, _hint(std::move(hint))
{
}

MenuState::~MenuState(void)
{
	_close();
}

int MenuState::getSelection(void) const
{
	return _menu ? _menu->getSelection() : 0;
}

std::unique_ptr<Font> MenuState::_createText(const std::string& text, Color tint, float scale, float centerY)
{
	auto font = std::make_unique<Font>();
	if (!font->loadFromJSON(fontPath())) {
		DEBUG_MSG(("Failed to load bitmap font from: " + fontPath() + "\n").c_str());
		return nullptr;
	}

	font->setText(text);
	font->setTint(tint);
	font->setScale(scale, scale);
	// Centered horizontally, and vertically on centerY
	const float centerX = Engine2D::getRenderer()->getWidth() / 2.0f;
	const float width = font->getTextWidth() * font->getScale().x;
	const float height = font->getHeight() * font->getScale().y;
	font->setPosition(vector2(centerX - width / 2.0f, centerY - height / 2.0f));
	font->setVisibility(true);
	_overlayList->push_back(font.get());
	return font;
}

void MenuState::onEnter(State* prev)
{
	// GameState::onEnter is skipped on purpose: an overlay has no objects or
	// collisions, only its screen-space text.
	(void)prev;

	IRenderer* renderer = Engine2D::getRenderer();
	if (!renderer) {
		return;
	}

	_overlayList = renderer->createRenderList(true); // true = screenSpace
	_titleText = _createText(_title, _titleTint, kTitleScale, kTitleCenterY);

	// The menu gets its own screen-space list, created after this one, so it
	// draws in front of the title and hint.
	_menu = Widgets::createMenu(renderer, fontPath(), vector2(renderer->getWidth() / 2.0f, kMenuTopY));
	for (const std::string& option : _options) {
		_menu->addItem(option);
	}

	_hintText = _createText(_hint, kHintTint, kHintScale, kHintCenterY);

	// A cursor only with a pointer (touch-only iOS has no mouse). Its list
	// is created after the game's HUD list, so it draws on top of the menu.
	if (Engine2D::getInput()->getMouse()) {
		_cursor = std::make_unique<Cursor>();
		if (_cursor->load(BasePath("cursors.png"))) {
			_overlayList->push_back(_cursor->getImage());
		}
		else {
			_cursor.reset();
		}
	}
}

bool MenuState::onExecute(float time)
{
	(void)time;
	const InputMap& input = game.getInputMap();

	if (onShortcut(input)) {
		return false;
	}

	if (_menu) {
		if (input.pressed("UP")) {
			_menu->selectPrevious();
		}
		else if (input.pressed("DOWN")) {
			_menu->selectNext();
		}
	}

	if (input.pressed("CONFIRM")) {
		onChoose(getSelection());
		return false;
	}

	// Hovering an option with the mouse selects it; clicking chooses it.
	Mouse* mouse = Engine2D::getInput()->getMouse();
	if (_cursor && _menu && mouse) {
		_cursor->follow(*mouse);
		const int hoverIndex = _menu->hitTest(_cursor->getPosition());
		if (hoverIndex >= 0) {
			_menu->setSelection(hoverIndex);
			if (mouse->pressed(MouseButton::Left)) {
				onChoose(hoverIndex);
				return false;
			}
		}
	}

	if (_menu) {
		_menu->applyTint(kSelectedTint, kUnselectedTint);
	}
	return true; // Keep running (state stays on top)
}

void MenuState::onExit(State* next)
{
	(void)next;
	_close();
}

void MenuState::_close(void)
{
	// Take everything out of the list before freeing it, then the list.
	if (_overlayList) {
		_overlayList->clear();
	}
	_titleText.reset();
	_hintText.reset();
	_cursor.reset();
	_menu.reset();
	if (_overlayList) {
		if (IRenderer* renderer = Engine2D::getRenderer()) {
			renderer->destroyRenderList(_overlayList);
		}
		_overlayList = nullptr;
	}
}
