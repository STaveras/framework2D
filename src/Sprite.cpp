// File: Sprite.cpp

#include "Sprite.h"
#include "Engine2D.h"
#include "IRenderer.h"
#include "ITexture.h"
#include "FileSystem.h"

#include <fstream>

Sprite::Sprite(void):
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(NULL),
	_sourceRect({ -1, -1, -1, -1 }) {

}

// TODO
Sprite::Sprite(const Sprite& image) :
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(image.getTexture()),
	_sourceRect(image._sourceRect) {

}

Sprite::Sprite(ITexture* pImage, const RECT& srcRect):
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(pImage),
	_sourceRect(srcRect) {

}

Sprite::Sprite(const char* filePath, Color clearColor, const RECT& srcRect):
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(NULL) {
	this->load(filePath,clearColor,srcRect);
}

Sprite::~Sprite(void)
{
	this->unload();
}

vector2 Sprite::getRectCenter(void) const
{
	vector2 rectCenter;

	rectCenter.x = (_sourceRect.right - _sourceRect.left) / 2.0f;
	rectCenter.y = (_sourceRect.bottom - _sourceRect.top) / 2.0f;

	return rectCenter;
}

// TODO: should probably check if the rect is non-zero and return those values instead

float Sprite::getWidth(void) const
{
   return (float)getTexture()->getWidth();
}

float Sprite::getHeight(void) const
{
   return (float)getTexture()->getHeight();
}

void Sprite::center(void)
{
   setCenter(getRectCenter());
}

Sprite* Sprite::load(const char* filePath, Color clearColor, const RECT& srcRect)
{
	IRenderer* renderer = Engine2D::getInstance()->getRenderer();

	if (renderer)
	{
		_texture = renderer->CreateTexture(filePath, clearColor);

		if (_texture)
		{
			if (srcRect.top != -1 && srcRect.right != -1 && srcRect.bottom != -1 && srcRect.left != -1) {
				this->setSourceRect(srcRect);
			}
			else
			{
				RECT newSrcRect;
				newSrcRect.left = 0;
				newSrcRect.top = 0;
				newSrcRect.right = _texture->getWidth();
				newSrcRect.bottom = _texture->getHeight();
				this->setSourceRect(newSrcRect);
			}
			return this;
		}
	}
	return NULL;
}

void Sprite::unload(void)
{
	if (_texture)
		Engine2D::getInstance()->getRenderer()->DestroyTexture(_texture);
}

//void Sprite::Save(const char* filePath)
//{
//	std::ofstream ofl(filePath, std::ios_base::binary);
//
//	if (ofl.good())
//	{
//		size_t uiStrLength = strlen(_texture->getFilename()) + 1;
//		Color clrColor = _texture->getKeyColor();
//
//		ofl.write((char*)&uiStrLength, sizeof(size_t));
//		ofl.write(_texture->getFilename(), uiStrLength);
//		ofl.write((char*)&_sourceRect, sizeof(RECT));
//		ofl.write((char*)&clrColor, sizeof(Color));
//		ofl.write((char*)&_center, sizeof(vector2));
//	}
//
//	ofl.close();
//	ofl.clear();
//}
//
//Sprite* Sprite::load(const char* filePath)
//{
//	Sprite* sprite = NULL;
//	if (!strcmp(FileSystem::File::GetFileExtension(filePath), "spr"))
//	{
//		std::ifstream ifl(filePath);
//
//		if (ifl.good())
//		{
//			size_t uiStrLength;
//			char szPath[MAX_PATH];
//			RECT srcRect;
//			Color clrColor;
//			vector2 center;
//
//			ifl.read((char*)&uiStrLength, sizeof(size_t));
//			ifl.read(szPath, uiStrLength);
//			ifl.read((char*)&srcRect, sizeof(srcRect));
//			ifl.read((char*)&clrColor, sizeof(Color));
//			ifl.read((char*)&center, sizeof(vector2));
//
//			Sprite::load(szPath, clrColor, srcRect);
//			this->setCenter(center);
//			sprite = this;
//		}
//
//		ifl.close();
//		ifl.clear();
//	}
//	else
//		Sprite::load(filePath);
//
//	return sprite;
//}

// Author: Stanley Taveras