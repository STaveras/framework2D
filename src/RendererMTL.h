// File: RendererMTL.h
// Metal renderer for macOS and iOS. It draws into a CAMetalLayer: the GLFW
// window's content view on macOS (the window needs Window::ClientAPI::None),
// or the game view's layer on iOS (Window::getNativeView()).
//
// Like RendererGL, sprites are transformed on the CPU and batched by texture,
// each render list is drawn with its own (parallax) camera transform, and the
// logical resolution (getWidth() x getHeight()) is scaled to fit the drawable,
// letterboxed when the aspect ratios differ.
#ifdef __APPLE__
#ifndef _RENDERERMTL_H
#define _RENDERERMTL_H

#include "IRenderer.h"

#include <memory>

class Font;
class Sprite;
class Window;

class RendererMTL : public IRenderer
{
	// Metal objects live in RendererMTL.mm so this header stays plain C++.
	struct Impl;
	std::unique_ptr<Impl> _impl;
	Window* _window = nullptr;

	void _drawImage(Sprite* sprite, Color tint, const vector2& offset, bool screenSpace);
	void _drawFont(Font* font, Color tint, const vector2& offset);

protected:
	void _beginRenderList(const RenderList& renderList) override;
	void _endRenderList(const RenderList& renderList) override;
	void _renderSprite(Sprite* sprite, Color tint, const vector2& offset, const RenderList& renderList) override;
	void _renderFont(Font* font, Color tint, const vector2& offset, const RenderList& renderList) override;

public:
	explicit RendererMTL(Window* window);
	~RendererMTL(void) override;

	ITexture* createTexture(const char* szFilename, Color colorKey = 0) override;
	bool destroyTexture(const ITexture* texture) override;

	void initialize(void) override;
	void shutdown(void) override;
	void render(void) override;

	void setVerticalSync(bool vsyncEnabled) override;
};

#endif //_RENDERERMTL_H
#endif //__APPLE__
