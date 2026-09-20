// File: Cursor.cpp
// Author: Stanley Taveras
// Created: 3/15/2026
// Purpose: Cursor sprite handling for FantasySideScroller

#include "Cursor.h"
#include "Debug.h"
#include <cstdio>

namespace {
constexpr long kCursorFrameWidth = 6;
constexpr long kCursorFrameHeight = 6;
constexpr long kCursorCellStrideX = 10;
constexpr long kCursorCellStrideY = 8;
constexpr long kCursorCellOriginX = 1;
constexpr long kCursorCellOriginY = 1;
constexpr long kCursorSheetColumns = 7;
constexpr long kCursorIndex = 8; // one-based index in the cursor atlas
}

Cursor::Cursor()
	: _state(CursorState::IDLE)
	, _image(nullptr) {
}

Cursor::~Cursor()
{
	unload();
}

bool Cursor::load(const std::string& filePath)
{
	unload();

	_image = new Image(filePath.c_str(), 0, _makeCursorRect(kCursorIndex));
	// The renderer pins the sprite's `center` (hotspot) at its position: a
	// frame pixel (tx,ty) is drawn at position + (tx - center.x, ty - center.y).
	// The reference cursor is a 6x6 frame whose hotspot is its top-left pixel.
	_image->setCenter(vector2(0.0f, 0.0f));
	_image->setOffset(vector2(0.0f, 0.0f));
	_image->setVisibility(true);
	_applyStateToImage();
	return true;
}

void Cursor::unload()
{
	SAFE_DELETE(_image);
}

void Cursor::setState(CursorState state)
{
	_state = state;
	_applyStateToImage();
}

void Cursor::setIdle()
{
	setState(CursorState::IDLE);
}

void Cursor::setClicking(bool clicking)
{
	if (clicking) {
		setState(CursorState::CLICK);
	}
	else if (_state == CursorState::CLICK) {
		setIdle();
	}
}

void Cursor::setDragging(bool dragging)
{
	if (dragging) {
		setState(CursorState::DRAG);
	}
	else if (_state == CursorState::DRAG) {
		setIdle();
	}
}

void Cursor::updateFromMouse(Mouse* mouse)
{
	if (!mouse) {
		setIdle();
		return;
	}

	//// Output mouse position for debugging
	//char buffer[64];
	//sprintf_s(buffer, sizeof(buffer), "Pos: %f, %f", mouse->_x, mouse->_y);
	//DEBUG_MSG(buffer);

	if (mouse->buttonPressed(MOUSE_LEFT)) {
		setClicking(true);
	}
	else if (mouse->buttonDown(MOUSE_LEFT)) {
		setDragging(true);
	}
	else {
		setIdle();
	}
}

void Cursor::setPosition(const vector2& pos)
{
	if (_image) {
		_image->setPosition(pos);
	}
}

RECT Cursor::_makeCursorRect(long index) const
{
	RECT rect{};
	const long zeroBasedIndex = (index > 0) ? index - 1 : 0;
	const long column = zeroBasedIndex % kCursorSheetColumns;
	const long row = zeroBasedIndex / kCursorSheetColumns;
	rect.left = kCursorCellOriginX + (column * kCursorCellStrideX);
	rect.top = kCursorCellOriginY + (row * kCursorCellStrideY);
	rect.right = rect.left + kCursorFrameWidth;
	rect.bottom = rect.top + kCursorFrameHeight;
	return rect;
}

void Cursor::_applyStateToImage()
{
	if (!_image) {
		return;
	}

	// Index 8 is the intended 6x6 arrow in the cursor atlas. Keep the same
	// pointer visible for idle, click, and drag states.
	_image->setSrcRect(_makeCursorRect(kCursorIndex));
	_image->setVisibility(true);
}
