
#pragma once

#include "ITexture.h"
#include <d3d9.h>
#include <d3dx9tex.h>

class TextureD3D : public ITexture
{
	friend class RendererDX;

	IDirect3DTexture9*	m_pTexture;
	D3DXIMAGE_INFO		m_tImageInfo;

public:
	TextureD3D(const char* szFilename):ITexture(szFilename), m_pTexture(NULL){}
	~TextureD3D(void)
	{
		if(m_pTexture)
			m_pTexture->Release();
	}

	IDirect3DTexture9* getTexture(void) { return m_pTexture; }
	unsigned int GetWidth(void) const { return m_tImageInfo.Width; }
	unsigned int GetHeight(void) const { return m_tImageInfo.Height; }
};