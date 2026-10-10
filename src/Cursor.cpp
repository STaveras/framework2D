// File: Cursor.cpp
// Author: Stanley Taveras
// Created: 3/15/2026
// Purpose: Cursor sprite handling for FantasySideScroller

#include "Cursor.h"
#include "Debug.h"

#include "Engine2D.h"
#include "IRenderer.h"
#include "ITexture.h"
#include "Renderer.h"
#include "Window.h"

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
	: _state(CursorState::IDLE) {
}

Cursor::~Cursor()
{
	unload();
}

bool Cursor::load(const std::string& filePath)
{
	if (filePath.empty()) {
		DEBUG_MSG("Cursor::load: filePath is empty");
		return false;
	}
	
	unload();

	IRenderer* renderer = Engine2D::getRenderer();
	ITexture* texture = renderer ? renderer->createTexture(filePath.c_str()) : nullptr;
	if (!texture) {
		DEBUG_MSG(("Cursor::load: failed to load " + filePath + "\n").c_str());
		return false;
	}

	// Borrow the renderer's cached texture instead of loading our own: every
	// cursor shares cursors.png, and an Image that loads its file destroys the
	// shared texture when deleted, under any other cursor still on screen.
	_image = std::make_unique<Image>(texture, _makeCursorRect(kCursorIndex));

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
	_image.reset();
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

void Cursor::follow(const Mouse& mouse)
{
	// Screen-space lists are drawn at the renderer's logical resolution and
	// scaled to the client area, so scale client pixels down the same way to
	// keep the hotspot under the OS cursor.
	vector2 position = mouse.getPosition();
	IRenderer* renderer = Engine2D::getRenderer();
	Window* window = Renderer::mainWindow;
	if (renderer && window && renderer->getWidth() > 0 && renderer->getHeight() > 0 &&
		window->getClientWidth() > 0 && window->getClientHeight() > 0) {
		position.x *= (float)renderer->getWidth() / (float)window->getClientWidth();
		position.y *= (float)renderer->getHeight() / (float)window->getClientHeight();
	}
	setPosition(position);

	if (mouse.pressed(MouseButton::Left)) {
		setClicking(true);
	}
	else if (mouse.down(MouseButton::Left)) {
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

vector2 Cursor::getPosition() const
{
	return _image ? _image->getPosition() : vector2(0.0f, 0.0f);
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
