// File: SpriteManager.cpp
// Author: Stanley Taveras
// Created: 2/28/2010
// Modified: 10/26/2018

#include "SpriteManager.h"

void SpriteManager::SetVisibility(bool isVisible)
{
   for (Factory<Sprite>::factory_iterator i = this->Begin(); i != this->End(); i++)
      (*i)->setVisibility(isVisible);
}

// HACK: Change this bologna
Sprite* SpriteManager::CreateSprite(const char* szFilename, color clearColor, const RECT* srcRect)
{
   Sprite* sprite = this->Create();
   (dynamic_cast<Image*>(sprite))->Load(szFilename, clearColor, srcRect);

   return sprite;
}

void SpriteManager::DestroySprite(Sprite* pSprite)
{
   for (IRenderer::RenderList::iterator o = _RenderList->begin(); o != _RenderList->end(); o++)
      if ((*o) == pSprite)
         _RenderList->erase(o);

   this->Destroy(pSprite);
}