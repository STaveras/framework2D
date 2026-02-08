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

#include <cmath>

namespace {
D3DXVECTOR2 worldToScreen(const Camera* camera, const vector2& worldPosition)
{
	if (!camera) {
		return D3DXVECTOR2(worldPosition.x, worldPosition.y);
	}

	const vector2 cameraPosition = camera->getRenderPosition();
	const vector2 center = camera->getCenter();
	const float zoom = (camera->getZoom() > 0.0f) ? camera->getZoom() : 1.0f;
	const float rotation = camera->getRotation();
	const float cosTheta = std::cos(rotation);
	const float sinTheta = std::sin(rotation);

	vector2 translated = worldPosition - cameraPosition;
	vector2 rotated(
		(translated.x * cosTheta) - (translated.y * sinTheta),
		(translated.x * sinTheta) + (translated.y * cosTheta));

	if (camera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter) {
		return D3DXVECTOR2(
			center.x + (rotated.x * zoom),
			center.y + (rotated.y * zoom));
	}

	// Preserve legacy origin-oriented behavior.
	vector2 legacyTranslated = worldPosition - (cameraPosition - center);
	vector2 legacyRotated(
		(legacyTranslated.x * cosTheta) - (legacyTranslated.y * sinTheta),
		(legacyTranslated.x * sinTheta) + (legacyTranslated.y * cosTheta));
	return D3DXVECTOR2(legacyRotated.x * zoom, legacyRotated.y * zoom);
}
}

RendererDX::RendererDX(void) : 
	IRenderer(RENDERER_TYPE_DX),
	m_hWnd(NULL),
	m_pD3D(NULL),
	m_pD3DDevice(NULL),
	m_pD3DSprite(NULL) {

}

RendererDX::RendererDX(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync) : 
	IRenderer(RENDERER_TYPE_DX, nWidth, nHeight, bFullscreen, bVsync),
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
	vector2 worldPosition = image->getPosition() + vector2(offset.x, offset.y);
	D3DXVECTOR2 screenPosition = worldToScreen(m_pCamera, worldPosition);
	const float cameraZoom = (m_pCamera && m_pCamera->getZoom() > 0.0f) ? m_pCamera->getZoom() : 1.0f;
	const D3DXVECTOR2 spriteScale = image->getScale();
	const float mirrorSignX = (spriteScale.x < 0.0f) ? -1.0f : 1.0f;
	const float mirrorSignY = (spriteScale.y < 0.0f) ? -1.0f : 1.0f;

	D3DXVECTOR3 position;
	// Preserve legacy mirror semantics for DirectX sprites: mirrored axes
	// use signed draw-position, while zoom is handled separately via scale.
	position.x = screenPosition.x * mirrorSignX;
	position.y = screenPosition.y * mirrorSignY;
	position.z = zValue;

	D3DXVECTOR2 rectCenter = image->getRectCenter();
	D3DXVECTOR2 scale = spriteScale;
	scale.x *= cameraZoom;
	scale.y *= cameraZoom;
	D3DXVECTOR2 center = image->getCenter();

	D3DXMATRIX transform;
	D3DXMatrixTransformation2D(&transform, &rectCenter, 0.0f, &scale, &center, image->getRotation(), NULL);

	// Keep sprite pivot tied to sprite-local mirror/scale only.
	// Camera zoom should affect transform scale, not draw-origin center.
	D3DXVECTOR3 center3D = D3DXVECTOR3(
		image->getCenter().x * image->getScale().x,
		image->getCenter().y * image->getScale().y,
		0.0f);

	// No mipmaps, and nearest neighbor/point filtering 
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	m_pD3DSprite->SetTransform(&transform);
	m_pD3DSprite->Draw(((TextureD3D*)image->getTexture())->getTexture(),
		&image->getSrcRect(),
		&center3D, &position,
		tint._color);
}

// void RendererDX::_DrawFont(Font* pFont)
//{

//}

ITexture* RendererDX::createTexture(const char* szFilename, Color colorKey)
{
	ITexture* pTexture = _textureExists(szFilename);

	if (!pTexture)
	{
		pTexture = (ITexture*)new TextureD3D(szFilename);
		pTexture->setKeyColor(colorKey);

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

	IRenderer::render();

	// We should only actually render, if there is a change or update from the animations/sprites ergo game/application itself
	m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, m_ClearColor._color, 1.0f, 0);

	// Begin drawing the scene
	if (SUCCEEDED(m_pD3DDevice->BeginScene()))
	{
		// TODO : (Optional) 3D Rendering here

		// Draw sprites
		if (SUCCEEDED(m_pD3DSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_DEPTH_FRONTTOBACK)))
		{
			D3DXMATRIX viewMat;
			D3DXMatrixIdentity(&viewMat);
			m_pD3DDevice->SetTransform(D3DTS_VIEW, &viewMat);

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
//#ifdef _DEBUG
//		for (auto collidable : m_Collidables)
//		{
//			_drawCollisionShapeBounds(collidable, Color(1.0, 0.0, 0.0, 0.0));
//		}
//#endif
		m_pD3DDevice->EndScene();
	}
	m_pD3DDevice->Present(NULL, NULL, NULL, NULL);

//#if _DEBUG
//	m_Collidables.clear(); // Clear collidables after rendering
//#endif
}

#endif
