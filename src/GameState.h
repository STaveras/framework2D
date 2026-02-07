// File: GameState.h

#pragma once

#include "ProgramState.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "CollisionSystem.h"

#include "Game.h"

class GameState : public ProgramState
{
	virtual void _OnObjectAdded(const Event& e);
	virtual void _OnObjectRemoved(const Event& e);

	friend Game;

protected:

	IRenderer::RenderList* _renderList = NULL;

	InputManager     _inputManager;
	ObjectManager    _objectManager;
	CollisionSystem  _collisionSystem;

public:
	GameState(void) : ProgramState() { }
	~GameState(void) { }
	
	InputManager * getInputManager(void) { return &_inputManager; }
	ObjectManager* getObjectManager(void) { return &_objectManager; }

	// Allow gamestates to reject objects?
	bool addObject(GameObject* object);
	bool removeObject(GameObject* object);

	virtual void onEnter(State* prevState) override;
	virtual bool onExecute(float time) override;
	virtual void onExit(State* nextState) override;
};
// - Stan
