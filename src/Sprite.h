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
	 int _verticalUVScrollRegionTop = 0;
	 int _verticalUVScrollRegionHeight = 0;
	 float _verticalUVScrollOffset = 0.0f;

public:
	 struct UVScrollSegment {
		 float sourceTop = 0.0f;
		 float sourceBottom = 0.0f;
		 float destinationTop = 0.0f;
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
	 // Scroll this sprite's source rectangle vertically within a repeating atlas
	 // region. The renderer can return two segments when the image crosses the
	 // region's bottom edge.
	 void setVerticalUVScroll(int regionTop, int regionHeight, float offset);
	 size_t getVerticalUVScrollSegments(UVScrollSegment* segments, size_t capacity) const;

	 void center(void);

	 //Sprite* load(const char* filePath);
	 //void Save(const char* filePath);

	 virtual Sprite* load(const char* filePath, Color clearColor = 0, const RECT& srcRect = {-1,-1,-1,-1});
	 virtual void unload(void);
}Image;
#endif  
//_SPRITE_H
