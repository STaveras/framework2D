// File: RendererDX.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/24/2023

#if _WIN32

#include "RendererDX.h"
#include "Animation.h"
#include "Camera.h"
#include "FramePacer.h"
#include "Font.h"
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

#include <algorithm>
#include <cmath>

namespace {
LPDIRECT3DTEXTURE9 getFontPixelTexture(LPDIRECT3DDEVICE9 device)
{
	static LPDIRECT3DTEXTURE9 s_texture = NULL;
	if (!s_texture && device)
	{
		if (SUCCEEDED(device->CreateTexture(1, 1, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &s_texture, NULL)))
		{
			D3DLOCKED_RECT lockedRect;
			if (SUCCEEDED(s_texture->LockRect(0, &lockedRect, NULL, 0)))
			{
				*reinterpret_cast<DWORD*>(lockedRect.pBits) = 0xFFFFFFFF;
				s_texture->UnlockRect(0);
			}
		}
	}
	return s_texture;
}

}

RendererDX::RendererDX(void) : 
	IRenderer(RENDERER_TYPE_DX),
	m_hWnd(NULL),
	m_pD3D(NULL),
	m_pD3DDevice(NULL),
	m_pD3DSprite(NULL),
	m_pFrameQuery(NULL) {

}

RendererDX::RendererDX(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync) : 
	IRenderer(RENDERER_TYPE_DX, nWidth, nHeight, bFullscreen, bVsync),
	m_hWnd(hWnd),
	m_pD3D(NULL),
	m_pD3DDevice(NULL),
	m_pD3DSprite(NULL),
	m_pFrameQuery(NULL) {

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
	_releaseFrameQuery();

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

void RendererDX::setVerticalSync(bool vsyncEnabled)
{
	const bool changed = vsyncEnabled != m_bVerticalSync;
	IRenderer::setVerticalSync(vsyncEnabled);

	// PresentationInterval only takes effect through a device reset.
	if (changed && m_pD3DDevice && m_pD3DSprite)
		_attemptDeviceReset();
}

void RendererDX::_releaseFrameQuery(void)
{
	if (m_pFrameQuery)
	{
		m_pFrameQuery->Release();
		m_pFrameQuery = NULL;
	}
}

// D3D9 lets the driver queue several frames ahead of the GPU, and each queued
// frame delays when new input reaches the screen. Waiting on an event query
// issued after Present caps the queue at one frame. (IDirect3DDevice9Ex::
// SetMaximumFrameLatency would need a D3D9Ex device, which rejects the
// D3DPOOL_MANAGED textures this renderer creates.)
void RendererDX::_waitForGPU(void)
{
	if (!m_pFrameQuery && FAILED(m_pD3DDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pFrameQuery)))
	{
		m_pFrameQuery = NULL;
		return;
	}

	if (FAILED(m_pFrameQuery->Issue(D3DISSUE_END)))
		return;

	HRESULT hr;
	while ((hr = m_pFrameQuery->GetData(NULL, 0, D3DGETDATA_FLUSH)) == S_FALSE)
		SwitchToThread();

	if (hr == D3DERR_DEVICELOST)
		_releaseFrameQuery();
}

D3DXVECTOR2 RendererDX::_toScreen(const vector2& worldPosition, bool screenSpace,
	float parallaxX, float parallaxY, float parallaxOriginX, float parallaxOriginY) const
{
	if (screenSpace || !m_pCamera) {
		return D3DXVECTOR2(worldPosition.x, worldPosition.y);
	}

	const vector2 cameraPosition = _parallaxCameraPosition(
		vector2(parallaxX, parallaxY), vector2(parallaxOriginX, parallaxOriginY));
	const vector2 screenPosition = _worldToScreen(worldPosition, cameraPosition);
	return D3DXVECTOR2(screenPosition.x, screenPosition.y);
}

void RendererDX::_renderSprite(Sprite* sprite, Color tint, const vector2& offset, const RenderList& renderList)
{
	_drawImage(sprite, tint, D3DXVECTOR2(offset.x, offset.y), 0.0f, renderList.screenSpace,
		renderList.parallaxX, renderList.parallaxY,
		renderList.parallaxOriginX, renderList.parallaxOriginY);
}

void RendererDX::_renderFont(Font* font, Color tint, const vector2& offset, const RenderList& renderList)
{
	_drawFont(font, tint, D3DXVECTOR2(offset.x, offset.y), 0.0f, renderList.screenSpace,
		renderList.parallaxX, renderList.parallaxY,
		renderList.parallaxOriginX, renderList.parallaxOriginY);
}

// Why do we have offset? Center is already an offset...
void RendererDX::_drawImage(Sprite* image, Color tint, D3DXVECTOR2 offset, float zValue, bool screenSpace,
	float parallaxX, float parallaxY, float parallaxOriginX, float parallaxOriginY)
{
	vector2 resolvedPosition = image->getPosition() + vector2(offset.x, offset.y);
	D3DXVECTOR2 screenPosition = _toScreen(resolvedPosition, screenSpace,
		parallaxX, parallaxY, parallaxOriginX, parallaxOriginY);
	float cameraZoom = 1.0f;
	if (!screenSpace && m_pCamera && m_pCamera->getZoom() > 0.0f) {
		cameraZoom = m_pCamera->getZoom();
	}
	const D3DXVECTOR2 spriteScale = image->getScale();

	D3DXVECTOR3 position;
	position.x = screenPosition.x;
	position.y = screenPosition.y;
	position.z = zValue;

	D3DXVECTOR2 scale = spriteScale;
	scale.x *= cameraZoom;
	scale.y *= cameraZoom;
	D3DXVECTOR2 transformPivot(position.x, position.y);

	D3DXMATRIX transform;
	// Important: scale/rotate around the sprite's resolved screen anchor,
	// not a fixed rect-local point. This keeps tile spacing stable when zoom changes.
	D3DXMatrixTransformation2D(&transform, &transformPivot, 0.0f, &scale, &transformPivot, image->getRotation(), NULL);

	D3DXVECTOR3 center3D = D3DXVECTOR3(image->getCenter().x, image->getCenter().y, 0.0f);

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

void RendererDX::_drawFont(Font* font, Color tint, D3DXVECTOR2 offset, float zValue, bool screenSpace,
	float parallaxX, float parallaxY, float parallaxOriginX, float parallaxOriginY)
{
	if (!font || !m_pD3DSprite || !m_pD3DDevice) {
		return;
	}

	LPDIRECT3DTEXTURE9 pixelTexture = getFontPixelTexture(m_pD3DDevice);
	if (!pixelTexture) {
		return;
	}

	vector2 resolvedPosition = font->getPosition() + vector2(offset.x, offset.y);
	D3DXVECTOR2 screenPosition = _toScreen(resolvedPosition, screenSpace,
		parallaxX, parallaxY, parallaxOriginX, parallaxOriginY);
	float cameraZoom = 1.0f;
	if (!screenSpace && m_pCamera && m_pCamera->getZoom() > 0.0f) {
		cameraZoom = m_pCamera->getZoom();
	}
	const float pixelWidth = font->getScale().x * cameraZoom;
	const float pixelHeight = font->getScale().y * cameraZoom;
	const vector2 center = font->getCenter();
	const std::string& text = font->getText();

	if (pixelWidth <= 0.0f || pixelHeight <= 0.0f || text.empty()) {
		return;
	}

	screenPosition.x -= center.x * pixelWidth;
	screenPosition.y -= center.y * pixelHeight;

	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	D3DXMATRIX originalTransform;
	m_pD3DSprite->GetTransform(&originalTransform);

	float cursorX = screenPosition.x;
	float cursorY = screenPosition.y;
	const float lineHeight = std::max(font->getHeight(), 1) * pixelHeight;

	for (char c : text)
	{
		if (c == '\n')
		{
			cursorX = screenPosition.x;
			cursorY += lineHeight + pixelHeight;
			continue;
		}

		const std::vector<int>& bitmap = font->getBitmap(c);
		const int glyphWidth = std::max(font->getWidth(c), 0);
		const int glyphAdvance = std::max(font->getBitmapWidth(), 1);

		for (int row = 0; row < static_cast<int>(bitmap.size()); ++row)
		{
			const int rowBits = bitmap[row];
            for (int column = 0; column < glyphWidth; ++column)
            {
                const int bitIndex = column;
                if (((rowBits >> bitIndex) & 1) == 0) {
                    continue;
                }

				D3DXVECTOR2 scale(pixelWidth, pixelHeight);
				D3DXVECTOR2 translation(cursorX + (column * pixelWidth), cursorY + (row * pixelHeight));
				D3DXVECTOR3 spritePosition(0.0f, 0.0f, zValue);
				D3DXMATRIX transform;
				D3DXMatrixTransformation2D(&transform, NULL, 0.0f, &scale, NULL, 0.0f, &translation);
				m_pD3DSprite->SetTransform(&transform);
				m_pD3DSprite->Draw(pixelTexture, NULL, NULL, &spritePosition, tint._color);
			}
		}

		cursorX += (glyphAdvance + 1) * pixelWidth;
	}

	m_pD3DSprite->SetTransform(&originalTransform);
}

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

		_releaseFrameQuery();

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
	_releaseFrameQuery();

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

			_drawRenderLists(false);
			_drawRenderLists(true);

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
	FramePacer::presentBegin();
	m_pD3DDevice->Present(NULL, NULL, NULL, NULL);
	_waitForGPU();
	FramePacer::presentEnd();

//#if _DEBUG
//	m_Collidables.clear(); // Clear collidables after rendering
//#endif
}

#endif
