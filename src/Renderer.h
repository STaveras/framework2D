// File: Renderer.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 1/13/2022

#if !defined(_RENDERER_H_)
#define _RENDERER_H_

#pragma once

#include "IRenderer.h"
#include "Renderable.h"
#include "RendererDX.h"
#include "RendererVK.h"
#include "Window.h"

namespace Renderer
{
	extern Window *window;
	
	IRenderer* get(void);

	#ifdef _WIN32
	// This function will be replaced with generic functions and will allow you to select a renderer module, to allow better encapsulation and extensibility
	IRenderer* createDXRenderer(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync);
	#endif

	// Create a Vulkan renderer
	IRenderer* createVKRenderer(Window* window);
	
	void destroyRenderer(IRenderer* pRenderer);
  //void render(Renderable* r); // TODO: You can use it to draw stuff in real-time (kinda slow, but uses less overall memory)
}

#endif