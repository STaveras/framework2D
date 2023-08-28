// File: TextureMTL.h
#pragma once

#include "ITexture.h"
#ifdef __APPLE__
#include <Metal/Metal.h>

class TextureMTL : public ITexture
{
	friend class RendererMTL;

	id<MTLTexture> _texture;
	unsigned int _width;
	unsigned int _height;

public:
	TextureMTL(const char* szFilename): ITexture(szFilename), _texture(nil), _width(0), _height(0) {}
	~TextureMTL(void)
	{
		// Texture release logic if needed
	}

	id<MTLTexture> getTexture(void) { return _texture; }
	unsigned int getWidth(void) const { return _width; }
	unsigned int getHeight(void) const { return _height; }
};

#endif //__APPLE__
