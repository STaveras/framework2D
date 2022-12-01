// File: GameState.h
#pragma once
#include "ProgramState.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "Player.h"
#include "Camera.h"
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

	Player* _player = NULL; // Action; the actors
	Camera* _camera = NULL; // Camera

	InputManager     _inputManager;
	ObjectManager    _objectManager;

#ifdef _DEBUG
	float timer = 0.0f;
#endif

public:
	GameState(void) : ProgramState() {

		_player = Engine2D::getGame()->getPlayers()->create();
		_camera = new Camera;
	}

	~GameState(void) {

		delete _camera;

		Engine2D::getGame()->getPlayers()->destroy(_player);
	}

	ObjectManager* getObjectManager(void) { return &_objectManager; }

	// Allow gamestates to reject objects?
	bool addObject(GameObject* object);
	bool removeObject(GameObject* object);

	virtual void onEnter(void);
	virtual bool onExecute(float time);
	virtual void onExit(void);
};
// - Stan