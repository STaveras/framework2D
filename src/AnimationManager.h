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

	void Initialize(void) { _pRenderList = Engine2D::GetInstance()->GetRenderer()->CreateRenderList(); }
	void Update(float fTime);
	void Shutdown(void) { Engine2D::GetInstance()->GetRenderer()->DestroyRenderList(_pRenderList); this->Clear(); }
};
// Author: Stanley Taveras