
#pragma once

#include "ITexture.h"
#ifdef _WIN32
#include <d3d9.h>
#include <d3dx9tex.h>

class TextureD3D : public ITexture
{
	friend class RendererDX;

	IDirect3DTexture9*	_texture;
	D3DXIMAGE_INFO		_imageInfo{};

public:
	TextureD3D(const char* szFilename): ITexture(szFilename), _texture(NULL) {}
	~TextureD3D(void)
	{
		if(_texture)
			_texture->Release();
	}

	IDirect3DTexture9* getTexture(void) { return _texture; }
	unsigned int getWidth(void) const { return _imageInfo.Width; }
	unsigned int getHeight(void) const { return _imageInfo.Height; }
};
#endif