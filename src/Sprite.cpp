// File: Sprite.cpp

#include "Sprite.h"
#include "Engine2D.h"
#include "IRenderer.h"
#include "ITexture.h"
#include "FileSystem.h"

#include <algorithm>
#include <cmath>
#include <fstream>

Sprite::Sprite(void):
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(NULL),
	_sourceRect({ -1, -1, -1, -1 }),
	_manuallyLoaded(false) {

}

// TODO
Sprite::Sprite(const Sprite& image) :
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(image.getTexture()),
	_sourceRect(image._sourceRect),
	_manuallyLoaded(false) {

}

Sprite::Sprite(ITexture* pImage, const RECT& srcRect):
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(pImage),
	_sourceRect(srcRect),
	_manuallyLoaded(false) {

}

Sprite::Sprite(const char* filePath, Color clearColor, const RECT& srcRect):
	Renderable(RENDERABLE_TYPE_SPRITE),
	_texture(NULL),
	_sourceRect(srcRect),
	_manuallyLoaded(true) {
	this->load(filePath,clearColor,srcRect);
}

void Sprite::setVerticalUVScroll(int regionTop, int regionHeight, float offset)
{
	_verticalUVScrollRegionTop = regionTop;
	_verticalUVScrollRegionHeight = regionHeight;
	_verticalUVScrollOffset = offset;
}

size_t Sprite::getVerticalUVScrollSegments(UVScrollSegment* segments, size_t capacity) const
{
	if (!segments || capacity == 0) {
		return 0;
	}

	const float sourceHeight = static_cast<float>(_sourceRect.bottom - _sourceRect.top);
	const bool canScroll =
		_verticalUVScrollRegionHeight > 0 &&
		sourceHeight > 0.0f &&
		sourceHeight <= static_cast<float>(_verticalUVScrollRegionHeight) &&
		_sourceRect.top >= _verticalUVScrollRegionTop &&
		_sourceRect.bottom <= _verticalUVScrollRegionTop + _verticalUVScrollRegionHeight;
	if (!canScroll) {
		segments[0] = { static_cast<float>(_sourceRect.top), static_cast<float>(_sourceRect.bottom), 0.0f, 1.0f };
		return 1;
	}

	const float regionHeight = static_cast<float>(_verticalUVScrollRegionHeight);
	float wrappedOffset = std::fmod(
		static_cast<float>(_sourceRect.top - _verticalUVScrollRegionTop) + _verticalUVScrollOffset,
		regionHeight);
	if (wrappedOffset < 0.0f) {
		wrappedOffset += regionHeight;
	}

	// This sheet uses nearest-neighbour sampling, so scroll in whole source
	// pixels. Round symmetrically so Up and Down move at the same rate.
	const int scrollPixels = static_cast<int>(std::floor(wrappedOffset + 0.5f)) % _verticalUVScrollRegionHeight;
	const float sourceTop = static_cast<float>(_verticalUVScrollRegionTop + scrollPixels);
	const float regionBottom = static_cast<float>(_verticalUVScrollRegionTop + _verticalUVScrollRegionHeight);
	const float firstHeight = std::min(sourceHeight, regionBottom - sourceTop);
	if (firstHeight >= sourceHeight) {
		segments[0] = { sourceTop, sourceTop + sourceHeight, 0.0f, 1.0f };
		return 1;
	}

	if (capacity < 2) {
		segments[0] = { static_cast<float>(_sourceRect.top), static_cast<float>(_sourceRect.bottom), 0.0f, 1.0f };
		return 1;
	}
	segments[0] = { sourceTop, regionBottom, 0.0f, firstHeight / sourceHeight };
	segments[1] = {
		static_cast<float>(_verticalUVScrollRegionTop),
		static_cast<float>(_verticalUVScrollRegionTop) + sourceHeight - firstHeight,
		firstHeight / sourceHeight,
		1.0f
	};
	return 2;
}

Sprite::~Sprite(void)
{
	if (_manuallyLoaded) {
		this->unload();
	}
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
		_texture = renderer->createTexture(filePath, clearColor);

		if (_texture)
		{
			if (srcRect.top != -1 && srcRect.right != -1 && srcRect.bottom != -1 && srcRect.left != -1) {
				this->setSrcRect(srcRect);
			}
			else
			{
				RECT newSrcRect;
				newSrcRect.left = 0;
				newSrcRect.top = 0;
				newSrcRect.right = _texture->getWidth();
				newSrcRect.bottom = _texture->getHeight();
				this->setSrcRect(newSrcRect);
			}
			return this;
		}
	}
	return NULL;
}

void Sprite::unload(void)
{
	if (_texture) {
		Engine2D::getInstance()->getRenderer()->destroyTexture(_texture);
	}
}

//void Sprite::Save(const char* filePath)
//{
//	std::ofstream ofl(filePath, std::ios_base::binary);
//
//	if (ofl.good())
//	{
//		size_t uiStrLength = strlen(_texture->getFileName()) + 1;
//		Color clrColor = _texture->getKeyColor();
//
//		ofl.write((char*)&uiStrLength, sizeof(size_t));
//		ofl.write(_texture->getFileName(), uiStrLength);
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
