// File: Image.cpp
#include "Image.h"
#include "Engine2D.h"
#include "IRenderer.h"
#include <fstream>
Image::Image(void):
	Renderable(RENDERABLE_TYPE_IMAGE),
	m_pTexture(NULL), 
	_rotation(0.0f),
	m_Center(vector2(0,0)),
	m_Scale(vector2(1.0f,1.0f))
{}

Image::Image(ITexture* pImage):
	Renderable(RENDERABLE_TYPE_IMAGE),
	m_pTexture(pImage),
	_rotation(0.0f),
	m_Center(vector2(0,0)),
	m_Scale(vector2(1.0f,1.0f))
{}

// TODO
Image::Image(const Image& image):
   Renderable(RENDERABLE_TYPE_IMAGE),
   m_pTexture(image.getTexture()),
   _rotation(0.0f),
   m_Center(vector2(0, 0)),
   m_Scale(vector2(1.0f, 1.0f)),
   m_SrcRect(image.m_SrcRect)
{

}

Image::Image(const char* filePath, color clearColor, const RECT* srcRect):
	Renderable(RENDERABLE_TYPE_IMAGE),
	m_pTexture(NULL), 
	_rotation(0.0f),
	m_Center(vector2(0,0)),
	m_Scale(vector2(1.0f,1.0f))
{
	this->Load(filePath,clearColor,srcRect);
}

Image::~Image(void)
{
	this->Unload();
}

vector2 Image::GetRectCenter(void) const
{
	vector2 rectCenter;
	rectCenter.x = (m_SrcRect.left + m_SrcRect.right/2.0f);
	rectCenter.y = (m_SrcRect.top + m_SrcRect.bottom/2.0f);

	return rectCenter;
}

// TODO: should probably check if the rect is non-zero and return those values instead

float Image::getWidth(void) const
{
   return (float)getTexture()->GetWidth();
}

float Image::getHeight(void) const
{
   return (float)getTexture()->GetHeight();
}

void Image::Mirror(bool bHorizontal, bool bVertical)
{
	if(bHorizontal)
	{
		m_Scale.x = -m_Scale.x;
		m_Center.x = -m_Center.x;
	}
	if(bVertical)
	{
		m_Scale.y = -m_Scale.y;
		m_Center.y = -m_Center.y;
	}
}

void Image::center(void)
{
   SetCenter(GetRectCenter());
}

Image* Image::Load(const char* filePath, color clearColor, const RECT* srcRect)
{
	IRenderer* renderer = Engine2D::getInstance()->GetRenderer();
	if (renderer)
	{
		m_pTexture = renderer->CreateTexture(filePath, clearColor);

		if (m_pTexture)
		{
			if(srcRect) {
				this->SetSourceRect(*srcRect);
			}
			else
			{
				RECT newSrcRect;
				newSrcRect.left = 0;
				newSrcRect.top = 0;
				newSrcRect.right = m_pTexture->GetWidth();
				newSrcRect.bottom = m_pTexture->GetHeight();
				this->SetSourceRect(newSrcRect);
			}
			return this;
		}
	}
	return NULL;
}

void Image::Unload(void)
{
	if (m_pTexture)
		Engine2D::getInstance()->GetRenderer()->DestroyTexture(m_pTexture);
}

// Author: Stanley Taveras