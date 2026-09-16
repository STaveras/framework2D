// RendererGL.h
#pragma once

#include "IRenderer.h"
#include "Window.h"
#include <vector>

class Sprite;
class Animation;

class RendererGL : public IRenderer
{
	GLFWwindow* _window = nullptr;

	struct SpriteVertex { float x, y, u, v; unsigned char r, g, b, a; };
	std::vector<SpriteVertex> _vertices;
	unsigned int _batchTexture = 0;
	vector2 _viewMin, _viewMax;
	void _flushBatch();
	void _drawImage(Sprite* sprite, Color tint, vector2 offset);

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
