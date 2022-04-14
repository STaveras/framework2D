// File: GameState.h
#pragma once
#include "IProgramState.h"
#include "AnimationManager.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "Player.h"
#include "Camera.h"

class GameState : public IProgramState
{
   virtual void _OnObjectStateEnter(const Event& e);
   virtual void _OnObjectStateExit(const Event& e);
   virtual void _OnObjectAdded(const Event& e);
   virtual void _OnObjectRemoved(const Event& e);

   friend Game;

protected:
   Player*                _player = NULL; // Action; the actors
   Camera*                _camera = NULL; // Camera
   Factory<Sprite>        _sprites;
   IRenderer::RenderList* _renderList = NULL;

   // Should gamestate *be* all three of these? (inherit from, instead of having)
   AnimationManager _animationManager;
   InputManager     _inputManager;
   ObjectManager    _objectManager;

public:
    GameState(void) : IProgramState() { _player = new Player; _camera = new Camera; }
   ~GameState(void) { delete _camera; delete _player; }

   Sprite* addSprite(const char* filename, color clearColor = 0, RECT* srcRect = NULL);
   void removeSprite(Sprite* sprite);

   //void addObject(GameObject* object);
   //void removeObject(GameObject* object);

   virtual void onEnter(void);
   virtual void onExecute(float fTime);
   virtual void onExit(void);
};
// - Stan