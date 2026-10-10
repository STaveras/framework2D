// File: TextureMTL.mm
#ifdef __APPLE__
#if !__has_feature(objc_arc)
#error "TextureMTL.mm must be compiled with -fobjc-arc"
#endif

#include "TextureMTL.h"

#include "stb/stb_image.h"

#include <stdexcept>
#include <string>

namespace {
id<MTLTexture> makeTexture(id<MTLDevice> device, unsigned int width, unsigned int height, const unsigned char* rgba)
{
	MTLTextureDescriptor* descriptor =
		[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
		                                                   width:width
		                                                  height:height
		                                               mipmapped:NO];
	descriptor.usage = MTLTextureUsageShaderRead;
	id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
	if (texture) {
		[texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
		           mipmapLevel:0
		             withBytes:rgba
		           bytesPerRow:(NSUInteger)width * 4];
	}
	return texture;
}
}

TextureMTL::TextureMTL(const char* szFilename, id<MTLDevice> device, Color colorKey)
	: ITexture(szFilename)
{
	int texWidth = 0;
	int texHeight = 0;
	int texChannels = 0;
	stbi_uc* pixels = stbi_load(szFilename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	if (!pixels) {
		throw std::runtime_error(std::string("Failed to load texture image: ") + szFilename);
	}
	if (colorKey._color != 0) {
		for (size_t i = 0, count = static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight); i < count; ++i) {
			stbi_uc* pixel = pixels + i * 4;
			if (pixel[0] == colorKey.r && pixel[1] == colorKey.g && pixel[2] == colorKey.b) {
				pixel[3] = 0;
			}
		}
	}

	_width = static_cast<unsigned int>(texWidth);
	_height = static_cast<unsigned int>(texHeight);
	_texture = makeTexture(device, _width, _height, pixels);
	stbi_image_free(pixels);

	if (!_texture) {
		throw std::runtime_error(std::string("Failed to create Metal texture: ") + szFilename);
	}
}

TextureMTL::TextureMTL(const char* name, id<MTLDevice> device, unsigned int width, unsigned int height, const unsigned char* rgba)
	: ITexture(name), _width(width), _height(height)
{
	_texture = makeTexture(device, width, height, rgba);
	if (!_texture) {
		throw std::runtime_error(std::string("Failed to create Metal texture: ") + name);
	}
}

TextureMTL::~TextureMTL(void)
{
	_texture = nil;
}
#endif //__APPLE__
