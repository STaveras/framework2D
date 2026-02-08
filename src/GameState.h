// File: GameState.h

#pragma once

#include "ProgramState.h"
#include "InputManager.h"	
#include "ObjectManager.h"
#include "CollisionSystem.h"
#include "IRenderer.h"

#include "Game.h"

#include <unordered_map>
#include <unordered_set>

class GameState : public ProgramState
{
	virtual void _OnObjectAdded(const Event& e);
	virtual void _OnObjectRemoved(const Event& e);

	friend Game;

protected:

	IRenderer::RenderList* _renderList = NULL;
	IRenderer::RenderList* _defaultRenderList = NULL;
	std::unordered_map<GameObject*, IRenderer::RenderList*> _objectRenderRoutes;
	std::unordered_set<IRenderer::RenderList*> _knownRenderLists;

	InputManager     _inputManager;
	ObjectManager    _objectManager;
	CollisionSystem  _collisionSystem;

public:
	GameState(void) : ProgramState() { }
	~GameState(void) { }
	
	InputManager * getInputManager(void) { return &_inputManager; }
	ObjectManager* getObjectManager(void) { return &_objectManager; }
	const CollisionSystem* getCollisionSystem(void) const { return &_collisionSystem; }
	IRenderer::RenderList* getBaseRenderList() const;
	IRenderer::RenderList* getDefaultRenderList() const;
	void setDefaultRenderList(IRenderer::RenderList* list);
	void routeObjectToRenderList(GameObject* object, IRenderer::RenderList* list);
	void clearObjectRenderRoute(GameObject* object);
	void clearRenderRoutes();

	// Allow gamestates to reject objects?
	bool addObject(GameObject* object);
	bool removeObject(GameObject* object);

	virtual void onEnter(State* prevState) override;
	virtual bool onExecute(float time) override;
	virtual void onExit(State* nextState) override;
};
// - Stan
