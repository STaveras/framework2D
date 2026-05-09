// File: Cursor.cpp
// Author: Stanley Taveras
// Created: 3/15/2026
// Purpose: Cursor sprite handling for FantasySideScroller

#include "Cursor.h"
#include "../Debug.h"
#include <cstdio>

namespace {
constexpr long kCursorFrameWidth = 10;
constexpr long kCursorFrameHeight = 9;
constexpr long kCursorIdleTop = 0;
constexpr long kCursorClickTop = kCursorFrameHeight;
constexpr long kCursorDragTop = kCursorFrameHeight * 2;
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

	_image = new Image(filePath.c_str(), 0, _makeCursorRect(kCursorIdleTop));
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

RECT Cursor::_makeCursorRect(long top) const
{
	RECT rect{};
	rect.left = 0;
	rect.top = top;
	rect.right = rect.left + kCursorFrameWidth;
	rect.bottom = rect.top + kCursorFrameHeight;
	return rect;
}

void Cursor::_applyStateToImage()
{
	if (!_image) {
		return;
	}

	long top = kCursorIdleTop;
	if (_state == CursorState::CLICK) {
		top = kCursorClickTop;
	}
	else if (_state == CursorState::DRAG) {
		top = kCursorDragTop;
	}

	_image->setSrcRect(_makeCursorRect(top));
	_image->setVisibility(true);
}
