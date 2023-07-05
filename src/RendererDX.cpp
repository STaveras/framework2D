// File: RendererDX.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/24/2023

#if _WIN32

#include "RendererDX.h"
#include "Animation.h"
#include "Camera.h"
#include "Frame.h"
#include "Renderable.h"
#include "Sprite.h"
#include "TextureD3D.h"

#pragma comment(lib, "d3d9.lib")
#ifdef _DEBUG
#pragma comment(lib, "d3dx9d.lib")
#else
#pragma comment(lib, "d3dx9.lib")
#endif
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

RendererDX::RendererDX(void) : IRenderer(),
							   m_hWnd(NULL),
							   m_pD3D(NULL),
							   m_pD3DDevice(NULL),
							   m_pD3DSprite(NULL) {

}

RendererDX::RendererDX(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync) : IRenderer(nWidth, nHeight, bFullscreen, bVsync),
m_hWnd(hWnd),
m_pD3D(NULL),
m_pD3DDevice(NULL),
m_pD3DSprite(NULL) {

}

RendererDX::~RendererDX(void)
{
   this->shutdown();
}

// Checks if the device is lost, and attempts to reset it if it is.
bool RendererDX::_checkDeviceLost()
{
   HRESULT hr = m_pD3DDevice->TestCooperativeLevel();

   if (hr == D3DERR_DEVICELOST) {
      return true;
   }
   else if (hr == D3DERR_DEVICENOTRESET) {
      if (SUCCEEDED(_attemptDeviceReset())) {
         return false;
      }
      else {
         return true;
      }
   }

   return false;
}

HRESULT RendererDX::_attemptDeviceReset()
{
   // Release resources that are tied to the device
   m_pD3DSprite->OnLostDevice();

   D3DPRESENT_PARAMETERS D3DPP = _d3dPresentParams();

   // Attempt to reset the device
   HRESULT hr = m_pD3DDevice->Reset(&D3DPP);

   if (SUCCEEDED(hr)) 
   {
      // Reinitialize resources tied to the device
      m_pD3DSprite->OnResetDevice();
   }

   return hr;
}

D3DPRESENT_PARAMETERS RendererDX::_d3dPresentParams(void)
{
   D3DPRESENT_PARAMETERS D3DPP;
   ZeroMemory(&D3DPP, sizeof(D3DPP));

   D3DPP.Windowed = (!m_bFullScreen) ? TRUE : FALSE;
   D3DPP.SwapEffect = D3DSWAPEFFECT_DISCARD;
   D3DPP.BackBufferFormat = D3DFMT_UNKNOWN;
   D3DPP.BackBufferWidth = m_nWidth;
   D3DPP.BackBufferHeight = m_nHeight;
   D3DPP.PresentationInterval = (m_bVerticalSync) ? D3DPRESENT_INTERVAL_DEFAULT : D3DPRESENT_INTERVAL_IMMEDIATE;
   D3DPP.hDeviceWindow = m_hWnd;

   return D3DPP;
}

// Why do we have offset? Center is already an offset...
void RendererDX::_drawImage(Sprite* image, Color tint, D3DXVECTOR2 offset, float zValue)
{
   D3DXMATRIX transform;
   //D3DXMatrixTransformation2D(&transform, &image->getRectCenter(), 0.0f, &image->getScale(), &image->getCenter(), image->getRotation(), NULL);

   D3DXVECTOR2 rectCenter = image->getRectCenter();
   D3DXVECTOR2 scale = image->getScale();
   D3DXVECTOR2 center = image->getCenter();

   D3DXMatrixTransformation2D(&transform, &rectCenter, 0.0f, &scale, &center, image->getRotation(), NULL);

   D3DXVECTOR3 position;
   position.x = (image->getPosition().x + offset.x) * image->getScale().x;
   position.y = (image->getPosition().y + offset.y) * image->getScale().y;
   position.z = 0.0f; // Will eventually be used for z-effects

   // Depth for some reason? 
   D3DXVECTOR3 center3D = D3DXVECTOR3(image->getCenter().x * image->getScale().x, image->getCenter().y * image->getScale().y, 0.0f); 

   // No mipmaps, and nearest neighbor/point filtering 
   m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
   m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
   m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

   m_pD3DSprite->SetTransform(&transform);
   m_pD3DSprite->Draw(((TextureD3D*)image->getTexture())->getTexture(), 
                     &image->getSrcRect(),
                     &center3D, &position,
                     tint._color);
}

// void RendererDX::_DrawFont(Font* pFont)
//{

//}

ITexture *RendererDX::createTexture(const char *szFilename, Color colorKey)
{
   ITexture* pTexture = _textureExists(szFilename);

   if (!pTexture)
   {
      pTexture = (ITexture*)new TextureD3D(szFilename);
      pTexture->SetKeyColor(colorKey);

      m_Textures.store(pTexture);

      D3DXCreateTextureFromFileEx(
         m_pD3DDevice, szFilename,
         D3DX_DEFAULT_NONPOW2,
         D3DX_DEFAULT_NONPOW2,
         D3DX_DEFAULT, 0,
         D3DFMT_A8R8G8B8,
         D3DPOOL_MANAGED,
         D3DX_FILTER_POINT,
         D3DX_DEFAULT,
         (DWORD)colorKey._color,
         &((TextureD3D*)pTexture)->_imageInfo, NULL,
         &((TextureD3D*)pTexture)->_texture);
   }
   else {
       ((TextureD3D*)pTexture)->getTexture()->AddRef();
   }

   return pTexture;
}

void RendererDX::destroyTexture(ITexture* texture)
{
   ((TextureD3D*)texture)->getTexture()->Release();

   IRenderer::destroyTexture(texture);
}

void RendererDX::initialize(void)
{
   bool needsReset = true;

   if (m_pD3D == NULL)
   {
      m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
      needsReset = false;
   }

   D3DPRESENT_PARAMETERS D3DPP = _d3dPresentParams();

   if (!needsReset) 
   {
      if (m_bFullScreen) {
         D3DPP.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
      }
      else {
         D3DPP.FullScreen_RefreshRateInHz = 0; // find supported refreshrates
      }
      m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &D3DPP, &m_pD3DDevice);
   }
   else
   {
      // Release resources before resetting
      if (m_pD3DSprite)
      {
         m_pD3DSprite->Release();
         m_pD3DSprite = nullptr;
      }

      HRESULT hr = m_pD3DDevice->Reset(&D3DPP);
      if (FAILED(hr))
      {
         // Handle error accordingly
         return;
      }
   }

   if (m_pD3DDevice)
   {
      HRESULT hr = D3DXCreateSprite(m_pD3DDevice, &m_pD3DSprite);
      if (FAILED(hr))
      {
         // Handle error accordingly
         return;
      }

      D3DXMATRIX ortho2D;
      D3DXMatrixOrthoLH(&ortho2D, (float)m_nWidth, (float)m_nHeight, 0.0f, 1.0f);
      m_pD3DDevice->SetTransform(D3DTS_PROJECTION, &ortho2D);
   }
}

void RendererDX::shutdown(void)
{
	if (m_pD3DSprite)
	{
		m_pD3DSprite->Release();
		m_pD3DSprite = NULL;
	}

	if (m_pD3DDevice)
	{
		m_pD3DDevice->Release();
		m_pD3DDevice = NULL;
	}

	if (m_pD3D)
	{
		m_pD3D->Release();
		m_pD3D = NULL;
	}
}

void RendererDX::render(void)
{
	if (!m_pD3DDevice || this->_checkDeviceLost())
		return;

   // We should only actually render, if there is a change or update from the animations/sprites ergo game/application itself
	m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, m_ClearColor._color, 1.0f, 0);

	// Begin drawing the scene
	if (SUCCEEDED(m_pD3DDevice->BeginScene()))
	{
		// TODO : (Optional) 3D Rendering here

		// Draw sprites
		if (SUCCEEDED(m_pD3DSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_DEPTH_FRONTTOBACK)))
		{
			if (m_pCamera)
			{
				D3DXMATRIX viewMat;
				D3DXMatrixIdentity(&viewMat);

            D3DXMATRIX scaleMat;
            D3DXMatrixScaling(&scaleMat, m_pCamera->getZoom(), m_pCamera->getZoom(), 1.0f);

            D3DXMATRIX rotationMat;
            D3DXMatrixRotationZ(&rotationMat, m_pCamera->getRotation());

            D3DXVECTOR2 position = m_pCamera->getPosition() - m_pCamera->getCenter();

            D3DXVECTOR2 xAxis = D3DXVECTOR2(1, 0);
            D3DXVECTOR2 yAxis = D3DXVECTOR2(0, 1);

            viewMat._41 = -D3DXVec2Dot(&xAxis, &position);
            viewMat._42 = -D3DXVec2Dot(&yAxis, &position);

            viewMat = scaleMat * rotationMat * viewMat;

            m_pD3DDevice->SetTransform(D3DTS_VIEW, &viewMat);
         }

         if (!_RenderLists.empty())
         {
            for (unsigned int i = 0; i < _RenderLists.size(); i++)
            {
               for (RenderList::iterator o = _RenderLists.at(i)->begin(); o != _RenderLists.at(i)->end(); o++)
               {
                  // We should be making *absolutely* sure that nothing that makes it here is NULL to begin with
                  if ((*o)) 
                  {
                     if ((*o)->isVisible())
                     {
                        switch ((*o)->getRenderableType())
                        {
                        case RENDERABLE_TYPE_SPRITE:
                        {
                            Image* image = (Image*)(*o);
                            _drawImage(image, image->getTintColor(),
                                              image->getOffset());
                        }
                        break;
                        case RENDERABLE_TYPE_ANIMATION:
                        {
                           Animation* animation = (Animation*)(*o);
                           if (animation->getFrameCount()) {
                              _drawImage(animation->getCurrentFrame()->getSprite(), 
                                         animation->getCurrentFrame()->getSprite()->getTintColor(),
                                         animation->getOffset());
                           }
                        }
                        break;
                        }
                     }
                  }
               }
            }
         }

         m_pD3DSprite->End();
      }

      // TODO : Font rendering here

      m_pD3DDevice->EndScene();
   }

   m_pD3DDevice->Present(NULL, NULL, NULL, NULL);
}

#endif