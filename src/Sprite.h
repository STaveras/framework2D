// File: Sprite.h
#if !defined(_SPRITE_H)
#define _SPRITE_H

#include "Renderable.h"
#include "Types.h"
#include "Collidable.h"

class ITexture;

typedef class Sprite : public Renderable
{
	 const ITexture* _texture;
	 RECT _sourceRect;
	 bool _manuallyLoaded;

public:
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

	 void center(void);

	 //Sprite* load(const char* filePath);
	 //void Save(const char* filePath);

	 virtual Sprite* load(const char* filePath, Color clearColor = 0, const RECT& srcRect = {-1,-1,-1,-1});
	 virtual void unload(void);
}Image;
#endif  //_SPRITE_H
// Author: Stanley Taveras