// File: RendererDX.h
#if !defined(_RENDERERD3D_H)
#define _RENDERERD3D_H
#include "IRenderer.h"
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
class Appearance;
class Image;
class RendererDX : public IRenderer
{
	bool				m_bFullscreen;
	bool				m_bVsync;
	HWND				m_hWnd;
	LPDIRECT3D9			m_pD3D;
	LPDIRECT3DDEVICE9	m_pD3DDevice;
	LPD3DXSPRITE		m_pD3DSprite;

	void _DrawAppearance(Appearance* pAppearance);
	void _DrawImage(Image* pSprite, color tint = 0xFFFFFFFF, D3DXVECTOR2 offset = D3DXVECTOR2(0,0));

public:
	RendererDX(void);
	RendererDX(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync);
	~RendererDX(void);

	HWND GetHWND(void) const { return m_hWnd; }
	LPDIRECT3D9 GetD3D(void) const { return m_pD3D; }
	LPDIRECT3DDEVICE9 GetD3DDevice(void) const { return m_pD3DDevice; }
	LPD3DXSPRITE GetD3DXSprite(void) const { return m_pD3DSprite; }

    enum ENUM
    {
        PIXEL_SHADER,
        VERTEX_SHADER
    };

private:
    void CreateShader(const char* file, int*);
	ITexture* CreateTexture(const char* szFilename, color colorKey = 0);

	void Initialize(void);
	void Shutdown(void);
	void Render(void);
};
#endif  //_RENDERER_H
// Author: Stanley Taveras