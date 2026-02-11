// File: IRenderer.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 4/1/2022

#include "IRenderer.h"
#include "Camera.h"
#include "Timer.h"

#include "Engine2D.h"
#include <algorithm>

IRenderer::~IRenderer() {
	m_Textures.clear();
}

// NOTE: Initial test demo
void IRenderer::_backgroundColorShift(void)
{
	if (m_bStaticBG || m_BackgroundColorPoints.size() < 2) {
		return;
	}

	Timer* timer = Engine2D::getTimer();
	if (!timer) {
		return;
	}

	m_BackgroundColorProgress += timer->getDeltaTime();
	while (m_BackgroundColorProgress >= 1.0f)
	{
		m_BackgroundColorProgress -= 1.0f;
		m_BackgroundColorStage = (m_BackgroundColorStage + 1) % m_BackgroundColorPoints.size();
	}

	const size_t nextStage = (m_BackgroundColorStage + 1) % m_BackgroundColorPoints.size();
	const Color startColor = m_BackgroundColorPoints[m_BackgroundColorStage];
	const Color endColor = m_BackgroundColorPoints[nextStage];

	const float t = std::clamp(m_BackgroundColorProgress, 0.0f, 1.0f);
	Color interpolatedColor = m_ClearColor;

	const auto lerpChannel = [t](byte start, byte end) -> byte {
		return static_cast<byte>(static_cast<float>(start) + (static_cast<float>(end) - static_cast<float>(start)) * t);
	};

	interpolatedColor.r = lerpChannel(startColor.r, endColor.r);
	interpolatedColor.g = lerpChannel(startColor.g, endColor.g);
	interpolatedColor.b = lerpChannel(startColor.b, endColor.b);
	interpolatedColor.a = lerpChannel(startColor.a, endColor.a);

	m_ClearColor = interpolatedColor;
}

ITexture* IRenderer::_textureExists(const char* szFilename)
{
	Factory<ITexture>::const_factory_iterator itr = m_Textures.begin();

	for (; itr != m_Textures.end(); itr++)
	{
		if(!strcmp((*itr)->getFilename(), szFilename))
			return (*itr);
	}

	return NULL;
}

void IRenderer::setClearColor(Color clearColor)
{
	m_ClearColor = clearColor;
	//m_bStaticBG = true;
}

void IRenderer::setBackgroundColorPoints(const std::vector<Color>& points)
{
	if (points.size() < 2) {
		return;
	}

	m_BackgroundColorPoints = points;
	resetBackgroundColorShift();
}

void IRenderer::resetBackgroundColorShift(void)
{
	m_BackgroundColorStage = 0;
	m_BackgroundColorProgress = 0.0f;
	m_ClearColor = m_BackgroundColorPoints.front();
}

void IRenderer::setCamera(Camera* pCamera)
{
	m_pCamera = pCamera;
	
	if (pCamera) 
	{
		m_pCamera->setScreenWidth(m_nWidth);
		m_pCamera->setScreenHeight(m_nHeight);
	}
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
	_backgroundColorShift();
}
