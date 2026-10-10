// File: Cursor.h
// Author: Stanley Taveras
// Created: 3/15/2026
// Purpose: Encapsulated cursor class with sprite sheet support for FantasySideScroller

#pragma once

#include <memory>
#include <string>

#include "Mouse.h"
#include "Sprite.h"

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
	// Move to the mouse and show its left button as click/drag. The sprite is
	// drawn in a screen-space render list, so the position is in the
	// renderer's logical resolution, not client pixels.
	void follow(const Mouse& mouse);
	void setPosition(const vector2& pos);
	vector2 getPosition() const;

	CursorState getState() const { return _state; }
	Image* getImage() { return _image.get(); }
	const Image* getImage() const { return _image.get(); }

private:
	RECT _makeCursorRect(long index) const;
	void _applyStateToImage();

	CursorState _state;
	std::unique_ptr<Image> _image;
};
