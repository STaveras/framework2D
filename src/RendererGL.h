// RendererGL.h
#pragma once

#include "IRenderer.h"
#include "Window.h"

class Sprite;
class Animation;
class Font;

class RendererGL : public IRenderer
{
	GLFWwindow* _window = nullptr;

	void _drawImage(Sprite* sprite, Color tint, vector2 offset);
	void _drawFont(Font* font, Color tint, vector2 offset);

public:
	RendererGL();
	explicit RendererGL(Window* window);
	~RendererGL() override;

	ITexture* createTexture(const char* szFilename, Color colorKey = 0) override;
	bool destroyTexture(const ITexture* texture) override;

	void initialize(void) override;
	void shutdown(void) override;
	void render(void) override;

	void setVerticalSync(bool vsyncEnabled) override;
};
