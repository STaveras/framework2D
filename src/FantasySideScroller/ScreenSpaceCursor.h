// File: ScreenSpaceCursor.h
// Author: Stanley Taveras
// Created: 3/24/2026
// Purpose: Shared helper for drawing a screen-space cursor (PlayState HUD,
// PauseState overlay) so the sprite tracks the OS mouse at the same position.
#pragma once

#include "../Engine2D.h"
#include "../IMouse.h"
#include "../Renderer.h"
#include "../Window.h"
#include "../maths/Vector2.h"

// Convert mouse (client-area) coordinates into the renderer's logical
// screen-space coordinates.  Screen-space render lists are drawn in the
// renderer's logical resolution while mouse coordinates are relative to the
// actual client area; keeping the cursor in logical space lets the renderer's
// final scaling put its hotspot back under the OS cursor.
inline vector2 ClientToRenderCursorPosition(const vector2& clientPosition)
{
	IRenderer* renderer = Engine2D::getRenderer();
	Window* window = Renderer::mainWindow;
	if (!renderer || !window) {
		return clientPosition;
	}

	const float clientWidth = static_cast<float>(window->getClientWidth());
	const float clientHeight = static_cast<float>(window->getClientHeight());
	if (clientWidth <= 0.0f || clientHeight <= 0.0f ||
		renderer->getWidth() <= 0 || renderer->getHeight() <= 0) {
		return clientPosition;
	}

	return vector2(
		clientPosition.x * static_cast<float>(renderer->getWidth()) / clientWidth,
		clientPosition.y * static_cast<float>(renderer->getHeight()) / clientHeight);
}
