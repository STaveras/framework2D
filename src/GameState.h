// File: GameState.h
#pragma once
#include "IProgramState.h"
#include "AnimationManager.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "Player.h"
class GameState : public IProgramState
{
   Factory<Sprite> _sprites;

   void _OnObjectStateEnter(const Event& e);
   void _OnObjectStateExit(const Event& e);
   void _OnObjectAdded(const Event& e);
   void _OnObjectRemoved(const Event& e);

   friend Game;

protected:
   IRenderer::RenderList* _renderList;

   AnimationManager _animationManager;
   InputManager     _inputManager;
   ObjectManager    _objectManager;

public:
   GameState(void) : IProgramState() { }
   ~GameState(void) { }

   Sprite* AddSprite(const char* filename, color clearColor = 0, rect* srcRect = NULL);
   void removeSprite(Sprite* sprite);

   //void addObject(GameObject* object);
   //void removeObject(GameObject* object);

   virtual void onEnter(void);
   virtual void onExecute(float fTime);
   virtual void onExit(void);
};
// - Stan