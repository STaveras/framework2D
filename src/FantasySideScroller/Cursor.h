// File: Cursor.h
// Author: Stanley Taveras
// Created: 3/15/2026
// Purpose: Encapsulated cursor class with sprite sheet support for FantasySideScroller

#pragma once

#include <string>

#include "../IMouse.h"
#include "../Sprite.h"

enum class CursorState
{
	IDLE,
	CLICK,
	DRAG
};

class Cursor
{
public:
	Cursor();
	~Cursor();

	Cursor(const Cursor&) = delete;
	Cursor& operator=(const Cursor&) = delete;
	Cursor(Cursor&&) = delete;
	Cursor& operator=(Cursor&&) = delete;

	bool load(const std::string& filePath);
	void unload();

	void setState(CursorState state);
	void setIdle();
	void setClicking(bool clicking);
	void setDragging(bool dragging);
	void updateFromMouse(Mouse* mouse);
	void setPosition(const vector2& pos);

	CursorState getState() const { return _state; }
	Image* getImage() { return _image; }
	const Image* getImage() const { return _image; }

private:
	RECT _makeCursorRect(long top) const;
	void _applyStateToImage();

	CursorState _state;
	Image* _image;
};
