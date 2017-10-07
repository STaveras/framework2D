// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"

GameState::GameState(void):
	IProgramState()
{}

void GameState::OnEnter(void)
{
	Engine2D* engine = Engine2D::GetInstance();

	_AnimationManager.Initialize();
	_InputManager.Initialize(engine->GetEventSystem(), engine->GetInput());
}

void GameState::OnExecute(float fTime)
{
	_AnimationManager.Update(fTime);
	_InputManager.Update(fTime);
	_ObjectManager.Update(fTime);
}

void GameState::OnExit(void)
{
	_InputManager.Shutdown();
	_AnimationManager.Shutdown();
	_Sprites.Clear();
}

// Author: Stanley Taveras