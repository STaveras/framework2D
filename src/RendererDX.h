// File: RendererDX.h
#ifdef _WIN32
#if !defined(_RENDERERD3D_H)
#define _RENDERERD3D_H
#include "IRenderer.h"
class Appearance;
class Sprite;
class RendererDX : public IRenderer
{
   HWND              m_hWnd;
   LPDIRECT3D9			m_pD3D;
   LPDIRECT3DDEVICE9	m_pD3DDevice;
   LPD3DXSPRITE		m_pD3DSprite;

   void _drawImage(Sprite* pSprite, Color tint = 0xFFFFFFFF, D3DXVECTOR2 offset = D3DXVECTOR2(0, 0), float zValue = 0.0f);

   bool _checkDeviceLost(void);
   HRESULT _attemptDeviceReset(void);
   D3DPRESENT_PARAMETERS _d3dPresentParams(void);

public:
   RendererDX(void);
   RendererDX(HWND hWnd, int nWidth, int nHeight, bool bFullscreen = false, bool bVsync = false);
   ~RendererDX(void);

   HWND getHWND(void) const { return m_hWnd; }
   LPDIRECT3D9 getD3D(void) const { return m_pD3D; }
   LPDIRECT3DDEVICE9 getD3DDevice(void) const { return m_pD3DDevice; }
   LPD3DXSPRITE getD3DXSprite(void) const { return m_pD3DSprite; }

private:
   ITexture* createTexture(const char* szFilename, Color colorKey = 0);
   void destroyTexture(ITexture* texture);

   void initialize(void);
   void shutdown(void);
   void render(void);
};
#endif //_RENDERER_H
#endif
// Author: Stanley Taveras