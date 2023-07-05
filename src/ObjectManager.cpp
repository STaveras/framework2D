// File: ObjectManager.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "ObjectManager.h"
#include "GameObject.h"
#include "ObjectOperator.h"
#include "Engine2D.h"
#include "Collidable.h"
#include "EventSystem.h"
#include "CollisionEvent.h"
#include "Square.h"

#include <unordered_set>

// How we should start thinking about events:
// SYSTEM EVENTS: The backend, mechanical stuff that glues the ''engine'' together (out/in)
// SIMULATION EVENTS: Pertinent only to the objects, and their interactions with one another. (in/out)

struct pair_hash {
	template <class T1, class T2>
	std::size_t operator() (const std::pair<T1, T2>& p) const {
		return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
	}
};

void ObjectManager::update(float fTime)
{
	std::map<std::string, GameObject*>::iterator objectItr = m_mObjects.begin();

	std::unordered_set<std::pair<GameObject*, GameObject*>, pair_hash> checkedPairs;

	for (; objectItr != m_mObjects.end(); objectItr++)
	{
		GameObject* object = objectItr->second;
		object->update(fTime);

		// Skip checking for tile objects
		if (object->getType() == GameObject::GAME_OBJ_TILE)
			continue;

		// Apply ObjectOperators
		std::list<ObjectOperator*> removalList;
		for (ObjectOperator* objOperator : m_lsObjOperators) {
			if (objOperator->isEnabled() && !(*objOperator)(object))
				removalList.push_back(objOperator);
		}

		// Remove finished operators
		for (ObjectOperator* objOp : removalList) {
			m_lsObjOperators.remove(objOp);
			Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOp);
		}

		// Check for collisions
		Collidable* collidable = object->getCollidable();
		if (collidable) {
			for (auto& otherPair : m_mObjects) {
				GameObject* otherObject = otherPair.second;

				if (otherObject == object ||
					object->getType() == GameObject::GAME_OBJ_TILE ||
					checkedPairs.count({ object, otherObject }) ||
					checkedPairs.count({ otherObject, object }))
					continue;

				Collidable* otherCollidable = otherObject->getCollidable();
				if (otherCollidable && collidable->collidesWith(otherCollidable)) {
					Engine2D::getEventSystem()->sendEvent(CollisionEvent(object, otherObject));
#if _DEBUG
					if (Debug::dbgCollision) {
						char buffer[256]{ 0 };
						sprintf_s(buffer, "%s\np1{%f, %f}\nco1{%f,%f,%f,%f}\nr1{%f,%f}\n", objectItr->first.c_str(), object->getPosition().x, object->getPosition().y,
							collidable->getPosition().x, collidable->getPosition().y,
							((Square*)collidable)->getMax().x, ((Square*)collidable)->getMax().y,
							object->getRenderable()->getPosition().x, object->getRenderable()->getPosition().y);
						DEBUG_MSG(buffer);
						DEBUG_MSG("+\n")

							sprintf_s(buffer, "%s\np2{%f, %f}\nco2{%f,%f,%f,%f}\nr2{%f,%f}\n", this->getObjectName(otherObject).c_str(), otherObject->getPosition().x, otherObject->getPosition().y,
								otherCollidable->getPosition().x, otherCollidable->getPosition().y,
								((Square*)otherCollidable)->getMax().x, ((Square*)otherCollidable)->getMax().y,
								otherObject->getRenderable()->getPosition().x, otherObject->getRenderable()->getPosition().y);

						DEBUG_MSG(buffer);
						DEBUG_MSG("\n");
					}
#endif
				}
				checkedPairs.insert({ object, otherObject });
			}
		}
	}
}

void ObjectManager::removeObject(GameObject* object)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
	for (; itr != m_mObjects.end(); itr++) {
		if (itr->second == object) {
			if (object) {

				object->finish(); m_mObjects.erase(itr);

				Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object); 
				
				break;
			}
		}
	}
}

void ObjectManager::removeObject(const char* name)
{
	GameObject* object = NULL;
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
	for (; itr != m_mObjects.end(); itr++) {
		if (itr->first == name) {
			object = itr->second;

			if (object)
			{
				object->finish(); m_mObjects.erase(itr);

				Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object); 
				
				break;
			}
		}
	}
}

void ObjectManager::addObject(const char* name, GameObject* object)
{
	m_mObjects[name] = object;

	Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_ADDED, object);
}

// NOTE: Maybe use a map instead and add/remove the operators by name?

void ObjectManager::pushOperator(ObjectOperator* objOperation)
{
	m_lsObjOperators.push_back(objOperation);

	Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_ADDED, objOperation);
}

void ObjectManager::popOperator(void)
{
	ObjectOperator* objOperator = m_lsObjOperators.back(); m_lsObjOperators.pop_back();
	Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOperator);
}

void ObjectManager::clearOperators(void)
{
	while (!m_lsObjOperators.empty()) {
		this->popOperator();
	}
}

void ObjectManager::sendEvent(Event::event_key key, void* sender)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

	for (; itr != m_mObjects.end(); itr++) {
		itr->second->sendInput(key, sender);
	}
}
