// File: AnimationManager.h
#pragma once
#include "Animation.h"
#include "Factory.h"
#include "SpriteManager.h"
#include <vector>
class AnimationManager : public Factory<Animation>
{
	IRenderer::RenderList* _pRenderList;

public:
	~AnimationManager(void){}

	Animation* GetAnimation(const char* szName);
	Animation* LoadAnimationFromFile(const char* szFilename);
	Animation* CreateAnimation(const char* szName);
	Animation* CreateAnimation(const char* szName, const std::vector<Sprite*>& vSprites, int nTargetFPS = 60);
	void DestroyAnimation(Animation* pAnimation);
	void DestroyAnimation(const char* szName);	

	void Initialize(IRenderer::RenderList* renderList) { _pRenderList = renderList; /*_pRenderList = Engine2D::getInstance()->GetRenderer()->CreateRenderList();*/ }
	void Update(float fTime);
	void Shutdown(void) { /*Engine2D::getInstance()->GetRenderer()->DestroyRenderList(_pRenderList);*/ this->Clear(); }
};
// Author: Stanley Taveras