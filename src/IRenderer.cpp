// File: IRenderer.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 4/1/2022

#include "IRenderer.h"
#include "Camera.h"
#include "Timer.h"

#include "Engine2D.h"

IRenderer::~IRenderer() {
	m_Textures.clear();
}

// NOTE: Initial test demo
#if _DEBUG
void IRenderer::_BackgroundColorShift(void)
{
	if (!m_bStaticBG)
	{
		// static Timer timer;
		Timer* timer = Engine2D::getTimer();

		static bool bReverse = false;
		static short nStage = 0;
		static float fAccum = 0.0f;

		Color colors[] = {
			0xFF000000, // Black
			0xFFFF0000, // Red
			0xFFFFFF00, // Yellow
			0xFF00FF00, // Green
			0xFF0000FF, // Blue
			0xFFFF00FF, // Purple
			0xFFFFFFFF, // White
			m_ClearColor
		};

		colors[7].r = (byte)(colors[nStage].r + fAccum * (colors[nStage + 1].r - colors[nStage].r));
		colors[7].g = (byte)(colors[nStage].g + fAccum * (colors[nStage + 1].g - colors[nStage].g));
		colors[7].b = (byte)(colors[nStage].b + fAccum * (colors[nStage + 1].b - colors[nStage].b));
		colors[7].a = (byte)(colors[nStage].a + fAccum * (colors[nStage + 1].a - colors[nStage].a));

		fAccum += (float)(bReverse ? -(timer->getDeltaTime()) : timer->getDeltaTime());

		if (fAccum >= 1.0f)
		{
			nStage++;

			if (nStage >= 6)
			{
				nStage = 6;
				bReverse = !bReverse;
			}

			fAccum = 0.0f;
		}
		else if (fAccum <= 0.0f)
		{
			fAccum = 1.0f;

			if (nStage <= 0)
			{
				nStage = 0;
				bReverse = !bReverse;
			}

			nStage--;
		}

		m_ClearColor = colors[7];
		// timer.update();
	}
}
#endif

ITexture* IRenderer::_TextureExists(const char* szFilename)
{
	Factory<ITexture>::const_factory_iterator itr = m_Textures.begin();

	for (; itr != m_Textures.end(); itr++)
	{
		if(!strcmp((*itr)->getFilename(), szFilename))
			return (*itr);
	}

	return NULL;
}

void IRenderer::SetClearColor(Color clearColor)
{
	m_ClearColor = clearColor;
	//m_bStaticBG = true;
}

void IRenderer::SetCamera(Camera* pCamera)
{
	m_pCamera = pCamera;
	m_pCamera->SetScreenWidth(m_nWidth);
	m_pCamera->SetScreenHeight(m_nHeight);
}

bool IRenderer::destroyTexture(const ITexture* pTexture)
{
	if (pTexture)
	{
		Factory<ITexture>::const_factory_iterator itr = m_Textures.begin();

		for(;itr != m_Textures.end(); itr++)
		{
			if(pTexture == (*itr))
			{
				m_Textures.erase(itr);
				return true;
			}
		}
	}

	return false;
}

void IRenderer::render(void) {
#if _DEBUG
	_BackgroundColorShift();
#else
	throw std::runtime_error("RenderingInterface::render() unimplemented");
#endif
}