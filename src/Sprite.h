// File: Sprite.h
#if !defined(_SPRITE_H)
#define _SPRITE_H

#include "Renderable.h"
#include "Types.h"
#include "Collidable.h"

class IRenderer;
class ITexture;
typedef class Sprite : public Renderable
{
	 const ITexture* _texture;
	 RECT _sourceRect;
	 vector2 _center;
	 vector2 _scale;
	 float _rotation; // (no. of dimensions - 1)

public:
	 Sprite(void);
	 Sprite(const Sprite& image);
	 Sprite(ITexture* pImage, const RECT& srcRect = { -1,-1,-1,-1 });
	 Sprite(const char* filePath, Color clearColor = 0, const RECT& srcRect = {-1,-1,-1,-1});
	 ~Sprite(void);

	 const ITexture* getTexture(void) const { return _texture; }
	 const RECT& getSourceRect(void) const { return _sourceRect; }
	 float getRotation(void) const { return _rotation; }
	 vector2 getCenter(void) const { return _center; }
	 vector2 getScale(void) const { return _scale; }
	 vector2 getRectCenter(void) const;

	 float getWidth(void) const;
	 float getHeight(void) const;

	 void setTexture(ITexture* pTexture) { _texture = pTexture; }
	 void setRotation(float fRotation) { _rotation = fRotation; }
	 void setCenter(vector2 center) { _center = center; }
	 void setScale(vector2 scale) { _scale = scale; }
	 void setSourceRect(const RECT& srcRect) { _sourceRect = srcRect; }

	 void mirror(bool horizontal, bool vertical);

	 void center(void);

	 //Sprite* load(const char* filePath);
	 //void Save(const char* filePath);

	 virtual Sprite* load(const char* filePath, Color clearColor = 0, const RECT& srcRect = {-1,-1,-1,-1});
	 virtual void unload(void);
}Image;
#endif  //_SPRITE_H
// Author: Stanley Taveras