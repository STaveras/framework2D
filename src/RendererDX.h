// File: RendererDX.h
#ifdef _WIN32
#if !defined(_RENDERERD3D_H)
#define _RENDERERD3D_H
#include "IRenderer.h"
class Appearance;
class Font;
class Sprite;
class Text;
class RendererDX : public IRenderer
{
   HWND              m_hWnd;
   LPDIRECT3D9			m_pD3D;
   LPDIRECT3DDEVICE9	m_pD3DDevice;
   LPD3DXSPRITE		m_pD3DSprite;
   LPDIRECT3DQUERY9	m_pFrameQuery; // Event query used to wait for the GPU after Present

    void _drawFont(Font* font, Color tint = 0xFFFFFFFF, D3DXVECTOR2 offset = D3DXVECTOR2(0, 0), float zValue = 0.0f, bool screenSpace = false,
       float parallaxX = 1.0f, float parallaxY = 1.0f, float parallaxOriginX = 0.0f, float parallaxOriginY = 0.0f);
    void _drawImage(Sprite* pSprite, Color tint = 0xFFFFFFFF, D3DXVECTOR2 offset = D3DXVECTOR2(0, 0), float zValue = 0.0f, bool screenSpace = false,
       float parallaxX = 1.0f, float parallaxY = 1.0f, float parallaxOriginX = 0.0f, float parallaxOriginY = 0.0f);
   void _drawText(Text* text, Color tint = 0xFFFFFFFF, D3DXVECTOR2 offset = D3DXVECTOR2(0, 0));
   D3DXVECTOR2 _toScreen(const vector2& worldPosition, bool screenSpace,
      float parallaxX, float parallaxY, float parallaxOriginX, float parallaxOriginY) const;

protected:
   void _renderSprite(Sprite* sprite, Color tint, const vector2& offset, const RenderList& renderList) override;
   void _renderFont(Font* font, Color tint, const vector2& offset, const RenderList& renderList) override;

private:
   bool _checkDeviceLost(void);
   HRESULT _attemptDeviceReset(void);
   D3DPRESENT_PARAMETERS _d3dPresentParams(void);
   void _releaseFrameQuery(void);
   void _waitForGPU(void);

public:
   RendererDX(void);
   RendererDX(HWND hWnd, int nWidth, int nHeight, bool bFullscreen = false, bool bVsync = false);
   ~RendererDX(void);

   HWND getHWND(void) const { return m_hWnd; }
   LPDIRECT3D9 getD3D(void) const { return m_pD3D; }
   LPDIRECT3DDEVICE9 getD3DDevice(void) const { return m_pD3DDevice; }
   LPD3DXSPRITE getD3DXSprite(void) const { return m_pD3DSprite; }

   void setVerticalSync(bool vsyncEnabled) override;

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
