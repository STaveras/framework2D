// File: Renderer.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 1/13/2013

#if !defined(_RENDERER_H_)
#define _RENDERER_H_

#pragma once

#include "IRenderer.h"
#include "Renderable.h"
#include "RendererDX.h"

namespace Renderer
{
	IRenderer* Get(void);

	// This function will be replaced with generic functions and will allow you to select a renderer module, to allow better encapsulation and extensibility
	IRenderer* CreateDXRenderer(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync);
	void DestroyRenderer(IRenderer* pRenderer);
    void Render(Renderable* r); // You can use it to draw stuff in real-time (kinda slow, but uses less overall memory)
}

#endif