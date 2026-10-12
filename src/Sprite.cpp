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

namespace {
struct UVSpan {
	float sourceStart;
	float sourceEnd;
	float destinationStart;
	float destinationEnd;
};

// Splits one axis of a source rect into the spans to sample from a repeating region: one
// normally, two when the looped rect crosses the region's far edge.
size_t splitUVLoopAxis(int sourceStart, int sourceEnd, int regionStart, int regionEnd, float position, UVSpan spans[2])
{
	const int sourceSize = sourceEnd - sourceStart;
	const int regionSize = regionEnd - regionStart;
	if (regionSize <= 0 || sourceSize <= 0 || sourceSize > regionSize) {
		spans[0] = { static_cast<float>(sourceStart), static_cast<float>(sourceEnd), 0.0f, 1.0f };
		return 1;
	}

	float wrapped = std::fmod(position, static_cast<float>(regionSize));
	if (wrapped < 0.0f) {
		wrapped += static_cast<float>(regionSize);
	}

	// The sheets use nearest-neighbour sampling, so loop in whole source pixels. Round
	// symmetrically so both directions move at the same rate.
	const int start = regionStart + static_cast<int>(std::floor(wrapped + 0.5f)) % regionSize;
	const int firstSize = std::min(sourceSize, regionEnd - start);
	if (firstSize >= sourceSize) {
		spans[0] = { static_cast<float>(start), static_cast<float>(start + sourceSize), 0.0f, 1.0f };
		return 1;
	}

	const float split = static_cast<float>(firstSize) / static_cast<float>(sourceSize);
	spans[0] = { static_cast<float>(start), static_cast<float>(regionEnd), 0.0f, split };
	spans[1] = { static_cast<float>(regionStart), static_cast<float>(regionStart + sourceSize - firstSize), split, 1.0f };
	return 2;
}
}

void Sprite::setUVLoop(const RECT& region, vector2 position)
{
	_uvLoopRegion = region;
	_uvLoopPosition = position;
}

size_t Sprite::getUVSegments(UVSegment* segments, size_t capacity) const
{
	if (!segments || capacity == 0) {
		return 0;
	}

	UVSpan xSpans[2];
	UVSpan ySpans[2];
	size_t xCount = splitUVLoopAxis(_sourceRect.left, _sourceRect.right, _uvLoopRegion.left, _uvLoopRegion.right, _uvLoopPosition.x, xSpans);
	size_t yCount = splitUVLoopAxis(_sourceRect.top, _sourceRect.bottom, _uvLoopRegion.top, _uvLoopRegion.bottom, _uvLoopPosition.y, ySpans);
	if (xCount * yCount > capacity) {
		xCount = yCount = 1;
		xSpans[0] = { static_cast<float>(_sourceRect.left), static_cast<float>(_sourceRect.right), 0.0f, 1.0f };
		ySpans[0] = { static_cast<float>(_sourceRect.top), static_cast<float>(_sourceRect.bottom), 0.0f, 1.0f };
	}

	size_t count = 0;
	for (size_t y = 0; y < yCount; ++y) {
		for (size_t x = 0; x < xCount; ++x) {
			segments[count++] = {
				xSpans[x].sourceStart, ySpans[y].sourceStart, xSpans[x].sourceEnd, ySpans[y].sourceEnd,
				xSpans[x].destinationStart, ySpans[y].destinationStart, xSpans[x].destinationEnd, ySpans[y].destinationEnd
			};
		}
	}
	return count;
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
