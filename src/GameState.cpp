// File: GameState.cpp

#include "GameState.h"
#include "Engine2D.h"
#include "RuntimeProfile.h"

#include "GameObject.h"
#include "Tile.h"

#include <algorithm>
#include <vector>

Engine2D* engine = Engine2D::getInstance();

void GameState::placeRenderable(Renderable* renderable, IRenderer::RenderList* list)
{
	IRenderer::RenderList*& placedList = _renderableLists[renderable];
	if (placedList == list) {
		return;
	}
	if (placedList && _knownRenderLists.count(placedList) != 0) {
		placedList->remove(renderable);
	}
	list->push_back(renderable);
	placedList = list;
}

void GameState::unplaceRenderable(Renderable* renderable)
{
	auto placed = _renderableLists.find(renderable);
	if (placed == _renderableLists.end()) {
		return;
	}
	if (_knownRenderLists.count(placed->second) != 0) {
		placed->second->remove(renderable);
	}
	_renderableLists.erase(placed);
}

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

	// Lists dropped here are destroyed by their owners; forget what was placed in them.
	for (auto itr = _renderableLists.begin(); itr != _renderableLists.end();) {
		if (_knownRenderLists.count(itr->second) == 0) {
			itr = _renderableLists.erase(itr);
		}
		else {
			++itr;
		}
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
}

bool GameState::onExecute(float time)
{
	{
		RuntimeProfile::Scope profile(RuntimeProfile::Region::ObjectUpdate);
		_objectManager.update(time);
	}
	{
		RuntimeProfile::Scope profile(RuntimeProfile::Region::Collision);
		_collisionSystem.update(_objectManager, time);
	}

	return true; // We're still updating!!! ...Right?
}

void GameState::onExit(State* nextState)
{
	_collisionSystem.reset();

	engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_REMOVED, this, &GameState::_OnObjectRemoved);
	engine->getEventSystem()->unregister<GameState>(EVT_OBJECT_ADDED, this, &GameState::_OnObjectAdded);

	clearRenderRoutes();
	if (_renderList) {
		engine->getRenderer()->destroyRenderList(_renderList);
	}
	_defaultRenderList = NULL;
	_renderList = NULL;
	_renderableLists.clear();
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
			placeRenderable(renderable, targetRenderList);
		}
		if (renderable) {
			bool visible = state == currentState;
			if (visible && object->getType() == GameObject::GAME_OBJ_TILE) {
				visible = static_cast<Tile*>(object)->isLayerVisible();
			}
			renderable->setVisibility(visible);
		}
	}
}

void GameState::_OnObjectRemoved(const Event & e)
{
	GameObject* object = (GameObject*)e.getSender();
	if (!object) {
		return;
	}

	for (auto it = object->begin(); it != object->end(); ++it) {
		GameObject::GameObjectState* state = (GameObject::GameObjectState*)(*it);
		if (state && state->getRenderable()) {
			unplaceRenderable(state->getRenderable());
		}
	}

	_objectRenderRoutes.erase(object);
	object->finish();
}
