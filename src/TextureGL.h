// TextureGL.h
#pragma once

#include "ITexture.h"

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
