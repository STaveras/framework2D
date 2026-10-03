// File: SpriteManager.h
#pragma once

#include "Sprite.h"
#include "Engine2D.h"
#include "Factory.h"
#include "IRenderer.h"

typedef class SpriteManager ImageManager;

class SpriteManager : public Factory<Sprite> // Renderable, instead of containing a renderlist...?
{
	friend class AnimationManager;

public:
	Sprite* CreateSprite(const char* szFilename, Color clearColor = 0, const RECT& srcRect = { -1,-1,-1,-1 });
	void DestroySprite(Sprite* pSprite);

	void SetVisibility(bool isVisible);
};

// Author: Stanley Taveras