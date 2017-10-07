// File: SpriteManager.h
#pragma once
#include "Sprite.h"
#include "Engine2D.h"
#include "Factory.h"
#include "IRenderer.h"
class SpriteManager : public Factory<Sprite>
{
	friend class AnimationManager;
	IRenderer::RenderList* _RenderList;

public:
	SpriteManager(void):_RenderList(NULL){_RenderList = Engine2D::GetInstance()->GetRenderer()->CreateRenderList(); }
	~SpriteManager(void){ Engine2D::GetInstance()->GetRenderer()->DestroyRenderList(_RenderList); }

	Sprite* CreateSprite(const char* szFilename, color clearColor = 0, const rect* srcRect = NULL);
	void DestroySprite(Sprite* pSprite);

	void SetVisibility(bool isVisible);
};
// Author: Stanley Taveras