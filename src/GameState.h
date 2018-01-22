// File: GameState.h
#pragma once
#include "IProgramState.h"
#include "AnimationManager.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "Player.h"
class GameState : public IProgramState
{
protected:
   Factory<Sprite>   _Sprites;
   AnimationManager	_AnimationManager;
   InputManager		_InputManager;
   ObjectManager    _ObjectManager;

public:
   GameState(void);

   virtual void OnEnter(void);
   virtual void OnExecute(float fTime);
   virtual void OnExit(void);
};
// - Stan