// TextureGL.h
#pragma once

#include "ITexture.h"

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

class TextureGL : public ITexture
{
	GLuint _textureId = 0;
	unsigned int _width = 0;
	unsigned int _height = 0;

public:
	explicit TextureGL(const char* path);
	~TextureGL() override;

	unsigned int getWidth() const override { return _width; }
	unsigned int getHeight() const override { return _height; }

	GLuint getTextureId() const { return _textureId; }
};
