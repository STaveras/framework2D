// File: SpriteManager.cpp
// Author: Stanley Taveras
// Created: 2/28/2010
// Modified: 10/26/2018

#include "SpriteManager.h"

void SpriteManager::SetVisibility(bool isVisible)
{
   for (Factory<Sprite>::factory_iterator i = this->begin(); i != this->end(); i++)
      (*i)->setVisibility(isVisible);
}

// HACK: Change this bologna
Sprite* SpriteManager::CreateSprite(const char* szFilename, Color clearColor, const RECT& srcRect)
{
   Sprite* sprite = this->create();
   sprite->load(szFilename, clearColor, srcRect);

   return sprite;
}

void SpriteManager::DestroySprite(Sprite* pSprite)
{
   //for (IRenderer::RenderList::iterator o = _RenderList->begin(); o != _RenderList->end(); o++)
   //   if ((*o) == pSprite)
   //      _RenderList->erase(o);

   this->destroy(pSprite);
}