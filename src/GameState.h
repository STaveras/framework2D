// File: GameState.h
#pragma once
#include "IProgramState.h"
#include "AnimationManager.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "Player.h"
class GameState : public IProgramState
{
   Factory<Sprite> _sprites; // Should we just have this in the CPP?

   void _OnObjectStateEnter(const Event& e);
   void _OnObjectStateExit(const Event& e);

   friend Game;

protected:
   IRenderer::RenderList* _renderList;

   AnimationManager _animationManager;
   InputManager     _inputManager;
   ObjectManager    _objectManager;

public:
   GameState(void) : IProgramState() {}

   Sprite* AddSprite(const char* filename, color clearColor = 0, rect* srcRect = NULL);
   void RemoveSprite(Sprite* sprite);

   //void AddObject(GameObject* object);
   //void RemoveObject(GameObject* object);

   virtual void OnEnter(void);
   virtual void OnExecute(float fTime);
   virtual void OnExit(void);
};
// - Stan