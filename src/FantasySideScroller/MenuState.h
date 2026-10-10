// File: MenuState.h
// A menu overlay that sits on top of PlayState, so the world stays visible but
// frozen behind it: a title, a column of options, a hint line and, when there
// is a mouse, a cursor. The player's UP/DOWN actions move the selection and
// CONFIRM chooses it; hovering an option with the mouse selects it and
// clicking chooses it. Subclasses supply the text and act on the choice.

#pragma once

#include "../Color.h"
#include "../GameState.h"

#include <memory>
#include <string>
#include <vector>

class Cursor;
class Font;
class InputMap;
class Menu;

class MenuState : public GameState
{
	std::string _title;
	Color _titleTint;
	std::vector<std::string> _options;
	std::string _hint;

	// Screen-space list for the title, hint and cursor; the menu has its own,
	// drawn in front of it.
	IRenderer::RenderList* _overlayList = nullptr;
	std::unique_ptr<Font> _titleText;
	std::unique_ptr<Font> _hintText;
	std::unique_ptr<Menu> _menu;
	std::unique_ptr<Cursor> _cursor;

	std::unique_ptr<Font> _createText(const std::string& text, Color tint, float scale, float centerY);
	void _close(void);

protected:
	MenuState(std::string title, Color titleTint, std::vector<std::string> options, std::string hint);

	// The player chose option `index`; this usually leaves the state.
	virtual void onChoose(int index) = 0;
	// Inputs handled before the menu's own (e.g. PAUSE resumes). Return true
	// when the state was left, to stop running it this tick.
	virtual bool onShortcut(const InputMap& input) { (void)input; return false; }

	int getSelection(void) const;

public:
	~MenuState(void) override;

	void onEnter(State* prev) override;
	bool onExecute(float time) override;
	void onExit(State* next) override;
};
