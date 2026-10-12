// File: Sprite.h
#if !defined(_SPRITE_H)
#define _SPRITE_H

#include "Renderable.h"
#include "Types.h"
#include "Collidable.h"

#include <cstddef>

class ITexture;

typedef class Sprite : public Renderable
{
	 const ITexture* _texture;
	 RECT _sourceRect;
	 bool _manuallyLoaded;
	 RECT _uvLoopRegion = { 0, 0, 0, 0 };
	 vector2 _uvLoopPosition;

public:
	 // A piece of the sprite to draw: the source pixels to sample and the part of the
	 // sprite's box they fill, as fractions (0..1) of its width and height.
	 struct UVSegment {
		 float sourceLeft = 0.0f;
		 float sourceTop = 0.0f;
		 float sourceRight = 0.0f;
		 float sourceBottom = 0.0f;
		 float destinationLeft = 0.0f;
		 float destinationTop = 0.0f;
		 float destinationRight = 1.0f;
		 float destinationBottom = 1.0f;
	 };

	 Sprite(void);
	 Sprite(const Sprite& image);
	 Sprite(ITexture* pImage, const RECT& srcRect = { -1,-1,-1,-1 });
	 Sprite(const char* filePath, Color clearColor = 0, const RECT& srcRect = {-1,-1,-1,-1});
	 ~Sprite(void);

	 const ITexture* getTexture(void) const { return _texture; }
	 const RECT& getSrcRect(void) const { return _sourceRect; }
	 vector2 getRectCenter(void) const;

	 float getWidth(void) const;
	 float getHeight(void) const;

	 void setTexture(ITexture* pTexture) { _texture = pTexture; }
	 void setSrcRect(const RECT& srcRect) { _sourceRect = srcRect; }
	 // Samples the source rect from a repeating region of the sheet instead of its own spot.
	 // position is where in the region the sprite's top-left samples, in source pixels
	 // (wrapped). An axis on which the region has no size draws the source rect as is.
	 void setUVLoop(const RECT& region, vector2 position);
	 // The pieces to draw: one normally, up to four when the looped rect crosses the
	 // region's edges.
	 size_t getUVSegments(UVSegment* segments, size_t capacity) const;

	 void center(void);

	 //Sprite* load(const char* filePath);
	 //void Save(const char* filePath);

	 virtual Sprite* load(const char* filePath, Color clearColor = 0, const RECT& srcRect = {-1,-1,-1,-1});
	 virtual void unload(void);
}Image;
#endif  
//_SPRITE_H
