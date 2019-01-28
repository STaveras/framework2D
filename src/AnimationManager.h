// File: AnimationManager.h
#pragma once
#include "Animation.h"
#include "Factory.h"
#include "SpriteManager.h"
#include <vector>

class AnimationManager : public Factory<Animation>
{
	IRenderer::RenderList* _renderList;

public:
	~AnimationManager(void){}

	Animation* GetAnimation(const char* szName);
	Animation* CreateAnimation(const char* szName, std::vector<Sprite*>* vSprites = NULL, int nTargetFPS = 60);

	void DestroyAnimation(Animation* pAnimation);
	void DestroyAnimation(const char* szName);	

	void Initialize(IRenderer::RenderList* renderList) { _renderList = renderList; /*_pRenderList = Engine2D::getInstance()->GetRenderer()->CreateRenderList();*/ }
	void Update(float fTime);
	void Shutdown(void) { /*Engine2D::getInstance()->GetRenderer()->DestroyRenderList(_pRenderList);*/ this->Clear(); }
};
// Author: Stanley Taveras