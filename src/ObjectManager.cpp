// File: ObjectManager.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "ObjectManager.h"
#include "GameObject.h"
#include "ObjectOperator.h"
#include "Engine2D.h"
#include "EventSystem.h"

#include <unordered_set>
#include <vector>
#include <string>
#include <cstdio>

// How we should start thinking about events:
// SYSTEM EVENTS: The backend, mechanical stuff that glues the ''engine'' together (out/in)
// SIMULATION EVENTS: Pertinent only to the objects, and their interactions with one another. (in/out)

void ObjectManager::update(float fTime)
{
	auto& objects = m_mObjects;
	std::vector<ObjectOperator*> removalList;

	// 1) Update all objects and apply operators.
	for (auto objectItr = objects.begin(); objectItr != objects.end(); ++objectItr)
	{
		GameObject* object = objectItr->second;
		if (!object) {
			continue;
		}

		if (!object->isStatic()) {
			object->update(fTime);
		}
		else
			continue;

		for (ObjectOperator* objOperator : m_lsObjOperators) {
			if (objOperator->isEnabled() && !(*objOperator)(object))
				removalList.push_back(objOperator);
		}
	}

	// 2) Remove finished operators once.
	if (!removalList.empty()) {
		std::unordered_set<ObjectOperator*> removed;
		for (ObjectOperator* objOp : removalList) {
			if (!objOp || removed.find(objOp) != removed.end()) {
				continue;
			}
			removed.insert(objOp);
			m_lsObjOperators.remove(objOp);
			Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOp, Event::event_priority_immediate);
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

				Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object, Event::event_priority_immediate); 
				
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

				Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object, Event::event_priority_immediate); 
				
				break;
			}
		}
	}
}

void ObjectManager::addObject(const char* name, GameObject* object)
{
	std::string requestedName = (name && name[0] != '\0') ? std::string(name) : std::string("object");
	std::string resolvedName = requestedName;

	auto existing = m_mObjects.find(resolvedName);
	if (existing != m_mObjects.end() && existing->second != object) {
#if _DEBUG
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer),
			"ObjectManager key collision on '%s' (existing=%p incoming=%p). Auto-suffixing.\n",
			resolvedName.c_str(),
			existing->second,
			object);
		DEBUG_MSG(buffer);
#endif

		unsigned int suffix = 2;
		do {
			resolvedName = requestedName + "#" + std::to_string(suffix++);
			existing = m_mObjects.find(resolvedName);
		} while (existing != m_mObjects.end());
	}

	m_mObjects[resolvedName] = object;

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
