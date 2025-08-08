// File: GameState.h

#pragma once

#include "ProgramState.h"
#include "InputManager.h"	
#include "ObjectManager.h"

#include "Game.h"

class GameState : public ProgramState
{
	virtual void _OnObjectStateEnter(const Event& e);
	virtual void _OnObjectStateExit(const Event& e);
	virtual void _OnObjectAdded(const Event& e);
	virtual void _OnObjectRemoved(const Event& e);

	friend Game;

protected:

	IRenderer::RenderList* _renderList = NULL;

	InputManager     _inputManager;
	ObjectManager    _objectManager;

public:
	GameState(void) : ProgramState() { }
	~GameState(void) { }
	
	InputManager * getInputManager(void) { return &_inputManager; }
	ObjectManager* getObjectManager(void) { return &_objectManager; }

	// Allow gamestates to reject objects?
	bool addObject(GameObject* object);
	bool removeObject(GameObject* object);

	virtual void onEnter(void);
	virtual bool onExecute(float time);
	virtual void onExit(void);
};
// - Stan