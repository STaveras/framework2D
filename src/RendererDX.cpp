// File: RendererDX.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/24/2023

#if _WIN32

#include "RendererDX.h"
#include "Animation.h"
#include "Camera.h"
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

int snapToPixel(float value)
{
	return static_cast<int>(std::lround(value));
}

int sampleNearestIndex(int sourceSize, int outputSize, int outputIndex)
{
	if (sourceSize <= 0 || outputSize <= 0) {
		return 0;
	}

	const float sourceCoord =
		(((static_cast<float>(outputIndex) + 0.5f) * static_cast<float>(sourceSize)) /
		static_cast<float>(outputSize)) - 0.5f;

	int index = static_cast<int>(std::lround(sourceCoord));
	if (index < 0) {
		index = 0;
	}
	else if (index >= sourceSize) {
		index = sourceSize - 1;
	}

	return index;
}

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
void RendererDX::_drawImage(Sprite* image, Color tint, D3DXVECTOR2 offset, float zValue, bool screenSpace)
{
	vector2 resolvedPosition = image->getPosition() + vector2(offset.x, offset.y);
	D3DXVECTOR2 screenPosition;
	float cameraZoom = 1.0f;
	if (screenSpace) {
		screenPosition = D3DXVECTOR2(resolvedPosition.x, resolvedPosition.y);
	}
	else {
		screenPosition = worldToScreen(m_pCamera, resolvedPosition);
		if (m_pCamera && m_pCamera->getZoom() > 0.0f) {
			cameraZoom = m_pCamera->getZoom();
		}
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

void RendererDX::_drawFont(Font* font, Color tint, D3DXVECTOR2 offset, float zValue, bool screenSpace)
{
	if (!font || !m_pD3DSprite || !m_pD3DDevice) {
		return;
	}

	LPDIRECT3DTEXTURE9 pixelTexture = getFontPixelTexture(m_pD3DDevice);
	if (!pixelTexture) {
		return;
	}

	vector2 resolvedPosition = font->getPosition() + vector2(offset.x, offset.y);
	D3DXVECTOR2 screenPosition;
	float cameraZoom = 1.0f;
	if (screenSpace) {
		screenPosition = D3DXVECTOR2(resolvedPosition.x, resolvedPosition.y);
	}
	else {
		screenPosition = worldToScreen(m_pCamera, resolvedPosition);
		if (m_pCamera && m_pCamera->getZoom() > 0.0f) {
			cameraZoom = m_pCamera->getZoom();
		}
	}
	const float pixelWidth = std::fabs(font->getScale().x * cameraZoom);
	const float pixelHeight = std::fabs(font->getScale().y * cameraZoom);
	const vector2 center = font->getCenter();
	const std::string& text = font->getText();

	if (pixelWidth <= 0.0f || pixelHeight <= 0.0f || text.empty()) {
		return;
	}

	const int baseX = snapToPixel(screenPosition.x - (center.x * pixelWidth));
	const int baseY = snapToPixel(screenPosition.y - (center.y * pixelHeight));

	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	m_pD3DDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	D3DXMATRIX originalTransform;
	m_pD3DSprite->GetTransform(&originalTransform);

	const int lineHeight = std::max(1, snapToPixel(static_cast<float>(std::max(font->getHeight(), 1) + 1) * pixelHeight));
	const int glyphAdvance = std::max(font->getBitmapWidth(), 1);
	const int letterAdvance = std::max(1, snapToPixel(static_cast<float>(glyphAdvance + 1) * pixelWidth));
	int cursorX = baseX;
	int cursorY = baseY;

	for (char c : text)
	{
		if (c == '\n')
		{
			cursorX = baseX;
			cursorY += lineHeight;
			continue;
		}

		const std::vector<int>& bitmap = font->getBitmap(c);
		const int glyphWidth = std::max(font->getWidth(c), 0);
		const int glyphHeight = static_cast<int>(bitmap.size());
		const int outputWidth = (glyphWidth > 0) ? std::max(1, snapToPixel(static_cast<float>(glyphWidth) * pixelWidth)) : 0;
		const int outputHeight = (glyphHeight > 0) ? std::max(1, snapToPixel(static_cast<float>(glyphHeight) * pixelHeight)) : 0;

		for (int outputRow = 0; outputRow < outputHeight; ++outputRow)
		{
			const int sourceRow = sampleNearestIndex(glyphHeight, outputHeight, outputRow);
			const int rowBits = bitmap[sourceRow];

			for (int outputColumn = 0; outputColumn < outputWidth; ++outputColumn)
			{
				const int sourceColumn = sampleNearestIndex(glyphWidth, outputWidth, outputColumn);
				if (((rowBits >> sourceColumn) & 1) == 0) {
					continue;
				}

				D3DXVECTOR2 scale(1.0f, 1.0f);
				D3DXVECTOR2 translation(static_cast<float>(cursorX + outputColumn), static_cast<float>(cursorY + outputRow));
				D3DXVECTOR3 spritePosition(0.0f, 0.0f, zValue);
				D3DXMATRIX transform;
				D3DXMatrixTransformation2D(&transform, NULL, 0.0f, &scale, NULL, 0.0f, &translation);
				m_pD3DSprite->SetTransform(&transform);
				m_pD3DSprite->Draw(pixelTexture, NULL, NULL, &spritePosition, tint._color);
			}
		}

		cursorX += letterAdvance;
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
				const auto drawRenderLists = [this](bool screenSpace)
				{
					for (unsigned int i = 0; i < _RenderLists.size(); i++)
					{
						RenderList* renderList = _RenderLists.at(i);
						if (!renderList || renderList->screenSpace != screenSpace) {
							continue;
						}

						for (RenderList::iterator o = renderList->begin(); o != renderList->end(); o++)
						{
							if (!(*o) || !(*o)->isVisible()) {
								continue;
							}

							switch ((*o)->getRenderableType())
							{
							case RENDERABLE_TYPE_SPRITE:
							{
								Image* image = (Image*)(*o);
								_drawImage(image, image->getTintColor(), image->getOffset(), 0.0f, screenSpace);
							}
							break;
							case RENDERABLE_TYPE_ANIMATION:
							{
								Animation* animation = (Animation*)(*o);
								if (animation->getFrameCount()) {
									_drawImage(animation->getCurrentFrame()->getSprite(),
										animation->getCurrentFrame()->getSprite()->getTintColor(),
										animation->getOffset(), 0.0f, screenSpace);
								}
							}
							break;
							case RENDERABLE_TYPE_FONT:
							{
								Font* font = (Font*)(*o);
								_drawFont(font, font->getTintColor(), font->getOffset(), 0.0f, screenSpace);
							}
							break;
							default:
								break;
							}
						}
					}
				};

				drawRenderLists(false);
				drawRenderLists(true);
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
