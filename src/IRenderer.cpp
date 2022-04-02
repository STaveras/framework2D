// File: IRenderer.cpp
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 4/1/2022

#include "IRenderer.h"
#include "Camera.h"
#include "Timer.h"

IRenderer::~IRenderer() {
	m_Textures.Clear();
}

void IRenderer::_BackgroundColorShift(void)
{
	if (!m_bStaticBG)
    {
		static Timer timer;
        static bool bReverse = false;
        static short nStage = 0;
        static float fAccum = 0.0f;

		color colors[] = {
			0xFF000000, // Black
			0xFFFF0000, // Red
			0xFFFFFF00, // Yellow
			0xFF00FF00, // Green
			0xFF0000FF, // Blue
			0xFFFF00FF, // Purple
			0xFFFFFFFF, // White
			m_ClearColor
		};
        
		colors[7].r = (byte)(colors[nStage].r + fAccum * (colors[nStage+1].r - colors[nStage].r));
		colors[7].g = (byte)(colors[nStage].g + fAccum * (colors[nStage+1].g - colors[nStage].g));
		colors[7].b = (byte)(colors[nStage].b + fAccum * (colors[nStage+1].b - colors[nStage].b));
		colors[7].a = (byte)(colors[nStage].a + fAccum * (colors[nStage+1].a - colors[nStage].a));

		fAccum += (float)(bReverse ? -(timer.GetDeltaTime()) : timer.GetDeltaTime());

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
		timer.Update();
    }
}

ITexture* IRenderer::_TextureExists(const char* szFilename)
{
	Factory<ITexture>::const_factory_iterator itr = m_Textures.Begin();

	for(; itr != m_Textures.End(); itr++)
	{
		if(!strcmp((*itr)->GetFilename(), szFilename))
			return (*itr);
	}

	return NULL;
}

void IRenderer::SetCamera(Camera* pCamera)
{
	m_pCamera = pCamera;
	m_pCamera->SetScreenWidth(m_nWidth);
	m_pCamera->SetScreenHeight(m_nHeight);
}

bool IRenderer::DestroyTexture(const ITexture* pTexture)
{
	if(pTexture)
	{
		Factory<ITexture>::const_factory_iterator itr = m_Textures.Begin();

		for(;itr != m_Textures.End(); itr++)
		{
			if(pTexture == (*itr))
			{
				m_Textures.Erase(itr);
				return true;
			}
		}
	}

	return false;
}

void IRenderer::Render(void) {
	_BackgroundColorShift();
}