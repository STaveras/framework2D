// File: AnimationManager.h
#pragma once

#include "Animation.h"
#include "Engine2D.h"
#include "Factory.h"
#include "SpriteManager.h"

#include <vector>

class AnimationManager : public Factory<Animation>
{
public:
	Animation* GetAnimation(const char* szName);
	Animation* CreateAnimation(const char* szName, std::vector<Sprite*>* vSprites = NULL, int nTargetFPS = 60);

	void DestroyAnimation(Animation* pAnimation);
	void DestroyAnimation(const char* szName);

	void update(float fTime);
};
// Author: Stanley Taveras