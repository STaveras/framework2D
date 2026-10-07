// File: TextureMTL.h
// An RGBA8 Metal texture loaded with stb_image. Only RendererMTL.mm and
// TextureMTL.mm include this header; both are compiled with -fobjc-arc.
#pragma once

#include "ITexture.h"
#ifdef __APPLE__
#include <Metal/Metal.h>

class TextureMTL : public ITexture
{
	id<MTLTexture> _texture = nil;
	unsigned int _width = 0;
	unsigned int _height = 0;

public:
	// Throws std::runtime_error if the image cannot be loaded.
	TextureMTL(const char* szFilename, id<MTLDevice> device);
	// A width x height texture from tightly packed RGBA8 pixels.
	TextureMTL(const char* name, id<MTLDevice> device, unsigned int width, unsigned int height, const unsigned char* rgba);
	~TextureMTL(void) override;

	id<MTLTexture> getTexture(void) const { return _texture; }
	unsigned int getWidth(void) const override { return _width; }
	unsigned int getHeight(void) const override { return _height; }
};

#endif //__APPLE__
