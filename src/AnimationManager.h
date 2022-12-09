// File: AnimationManager.h
#pragma once

#include "Animation.h"
#include "Engine2D.h"
#include "Factory.h"
#include "SpriteManager.h"

#include <vector>

class AnimationManager : public Factory<Animation>
{
	//IRenderer::RenderList* _renderList;

public:
	AnimationManager(void) /*: _renderList(NULL)*/ {
		//if (!_renderList) {
		//	_renderList = Engine2D::getInstance()->GetRenderer()->createRenderList();
		//}
	}

	~AnimationManager(void) {
		//if (_renderList) { // Just to be proper *_*
		//	Engine2D::getInstance()->GetRenderer()->destroyRenderList(_renderList);
		//}
	}

	Animation* GetAnimation(const char* szName);
	Animation* CreateAnimation(const char* szName, std::vector<Sprite*>* vSprites = NULL, int nTargetFPS = 60);

	void DestroyAnimation(Animation* pAnimation);
	void DestroyAnimation(const char* szName);

	void update(float fTime);
};
// Author: Stanley Taveras