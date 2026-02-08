// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"

#include "GameObject.h"

#include <algorithm>
#include <vector>

namespace {
bool containsRenderable(IRenderer::RenderList* list, Renderable* renderable)
{
	if (!list || !renderable) {
		return false;
	}
	return std::find(list->begin(), list->end(), renderable) != list->end();
}
}

Engine2D* engine = Engine2D::getInstance();

bool GameState::addObject(GameObject * object)
{
	if (!object) {
		return false;
	}

	_objectManager.addObject("object", object);
	return true;
}

bool GameState::removeObject(GameObject * object)
{
	if (!object) {
		return false;
	}

	_objectManager.removeObject(object);
	return true;
}

IRenderer::RenderList* GameState::getBaseRenderList() const
{
	return _renderList;
}

IRenderer::RenderList* GameState::getDefaultRenderList() const
{
	return _defaultRenderList ? _defaultRenderList : _renderList;
}

void GameState::setDefaultRenderList(IRenderer::RenderList* list)
{
	_defaultRenderList = list ? list : _renderList;
	if (_renderList) {
		_knownRenderLists.insert(_renderList);
	}
	if (_defaultRenderList) {
		_knownRenderLists.insert(_defaultRenderList);
	}
}

void GameState::routeObjectToRenderList(GameObject* object, IRenderer::RenderList* list)
{
	if (!object) {
		return;
	}

	if (!list) {
		_objectRenderRoutes.erase(object);
		return;
	}

	_objectRenderRoutes[object] = list;
	_knownRenderLists.insert(list);
}

void GameState::clearObjectRenderRoute(GameObject* object)
{
	if (!object) {
		return;
	}
	_objectRenderRoutes.erase(object);
}

void GameState::clearRenderRoutes()
{
	_objectRenderRoutes.clear();
	_knownRenderLists.clear();
	if (_renderList) {
		_knownRenderLists.insert(_renderList);
	}
	if (_defaultRenderList) {
		_knownRenderLists.insert(_defaultRenderList);
	}
}

void GameState::onEnter(State* prevState)
{
	_renderList = engine->getRenderer()->createRenderList();
	_defaultRenderList = _renderList;
	clearRenderRoutes();
	_collisionSystem.reset();

	engine->getEventSystem()->registerCallback<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);
	engine->getEventSystem()->registerCallback<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);

	_inputManager.initialize(engine->getEventSystem(),
		engine->getInput());
}

bool GameState::onExecute(float time)
{
	_inputManager.update(time);
	_objectManager.update(time);
	_collisionSystem.update(_objectManager.getObjects());

	return true; // We're still updating!!! ...Right?
}

void GameState::onExit(State* nextState)
{
	_collisionSystem.reset();

	_inputManager.shutdown();

	engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);
	engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);

	clearRenderRoutes();
	if (_renderList) {
		engine->getRenderer()->destroyRenderList(_renderList);
	}
	_defaultRenderList = NULL;
	_renderList = NULL;
}

///
// Right now the only object manager that is being updated is this one since it's on the top of the stack...
// However, we'll need to change the event system to send details about the object and the state change.
// Meaning we'll need to filter for the object, checking if it's contained within our object manager
///

void GameState::_OnObjectAdded(const Event & e)
{
	GameObject* object = (GameObject*)e.getSender();
	if (!object) {
		return;
	}

	object->start();

	IRenderer::RenderList* targetRenderList = this->getDefaultRenderList();
	std::unordered_map<GameObject*, IRenderer::RenderList*>::const_iterator routeItr = _objectRenderRoutes.find(object);
	if (routeItr != _objectRenderRoutes.end() && routeItr->second) {
		targetRenderList = routeItr->second;
	}
	if (!targetRenderList) {
		targetRenderList = _renderList;
	}
	if (targetRenderList) {
		_knownRenderLists.insert(targetRenderList);
	}
	if (_renderList) {
		_knownRenderLists.insert(_renderList);
	}

	GameObject::GameObjectState* currentState = object->getState();
	for (auto it = object->begin(); it != object->end(); ++it) {
		GameObject::GameObjectState* state = (GameObject::GameObjectState*)(*it);
		if (!state) {
			continue;
		}
		Renderable* renderable = state->getRenderable();
		if (renderable && targetRenderList) {
			for (IRenderer::RenderList* knownList : _knownRenderLists) {
				if (!knownList || knownList == targetRenderList) {
					continue;
				}
				knownList->remove(renderable);
			}

			if (!containsRenderable(targetRenderList, renderable)) {
				targetRenderList->push_back(renderable);
			}
		}
		if (renderable) {
			renderable->setVisibility(state == currentState);
		}
	}
}

void GameState::_OnObjectRemoved(const Event & e)
{
	GameObject* object = (GameObject*)e.getSender();
	if (!object) {
		return;
	}

	std::vector<IRenderer::RenderList*> listsToPrune;
	listsToPrune.reserve(_knownRenderLists.size() + 2);
	if (_renderList) {
		listsToPrune.push_back(_renderList);
	}
	if (_defaultRenderList && _defaultRenderList != _renderList) {
		listsToPrune.push_back(_defaultRenderList);
	}
	for (IRenderer::RenderList* knownList : _knownRenderLists) {
		if (!knownList) {
			continue;
		}
		if (std::find(listsToPrune.begin(), listsToPrune.end(), knownList) == listsToPrune.end()) {
			listsToPrune.push_back(knownList);
		}
	}

	for (auto it = object->begin(); it != object->end(); ++it) {
		GameObject::GameObjectState* state = (GameObject::GameObjectState*)(*it);
		if (!state) {
			continue;
		}
		Renderable* renderable = state->getRenderable();
		if (!renderable) {
			continue;
		}
		for (IRenderer::RenderList* renderList : listsToPrune) {
			if (renderList) {
				renderList->remove(renderable);
			}
		}
	}

	_objectRenderRoutes.erase(object);
	object->finish();
}
