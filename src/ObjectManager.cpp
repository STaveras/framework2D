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
	// Use unordered_map for faster iteration if possible
	auto& objects = m_mObjects;
	std::unordered_set<uintptr_t> checkedPairs; // Use a hash of pointer pairs for faster lookup

	for (auto objectItr = objects.begin(); objectItr != objects.end(); ++objectItr)
	{
		GameObject* object = objectItr->second;
		object->update(fTime);

		if (object->getType() == GameObject::GAME_OBJ_TILE)
			continue;

		// Apply ObjectOperators
		std::vector<ObjectOperator*> removalList;
		for (ObjectOperator* objOperator : m_lsObjOperators) {
			if (objOperator->isEnabled() && !(*objOperator)(object))
				removalList.push_back(objOperator);
		}

		// Remove finished operators
		for (ObjectOperator* objOp : removalList) {
			m_lsObjOperators.remove(objOp);
			Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOp);
		}

		Collidable* collidable = object->getCollidable();
		if (collidable && collidable->isActive()) {

//#if _DEBUG && (defined(_WIN32) || defined(_WIN64))
//			RenderingInterface* renderer = dynamic_cast<IRenderer*>(Engine2D::getRenderer());
//			if (renderer->renderingAPI() == RenderingInterface::RENDERER_TYPE_DX) {
//				dynamic_cast<RendererDX*>(renderer)->m_Collidables.push_back(collidable);
//			}
//#endif
			for (auto& otherPair : objects) {
				GameObject* otherObject = otherPair.second;

				if (otherObject == object ||
					object->getType() == GameObject::GAME_OBJ_TILE)
					continue;

				// Use a unique hash for the pair to avoid std::pair overhead
				uintptr_t pairHash = reinterpret_cast<uintptr_t>(object) ^ reinterpret_cast<uintptr_t>(otherObject);
				if (checkedPairs.count(pairHash))
					continue;

				Collidable* otherCollidable = otherObject->getCollidable();
				if (otherCollidable && otherCollidable->isActive() && collidable->collidesWith(otherCollidable)) {

					Engine2D::getEventSystem()->sendEvent(CollisionEvent(object, otherObject));
#if _DEBUG
					if (Debug::dbgObjects) {
//#if (defined(_WIN32) || defined(_WIN64))
//						if (renderer->renderingAPI() == RenderingInterface::RENDERER_TYPE_DX) {
//							dynamic_cast<RendererDX*>(renderer)->m_Collidables.push_back(otherCollidable);
//						}
//#endif
						char buffer[256]{ 0 };
						sprintf_s(buffer, sizeof(buffer), "%s\np1{%f, %f}\nco1{%f,%f,%f,%f}\nr1{%f,%f}\n", objectItr->first.c_str(), object->getPosition().x, object->getPosition().y,
							collidable->getPosition().x, collidable->getPosition().y,
							((Square*)collidable)->getMax().x, ((Square*)collidable)->getMax().y,
							object->getRenderable()->getPosition().x, object->getRenderable()->getPosition().y);
						DEBUG_MSG(buffer);
						DEBUG_MSG("+\n")
						sprintf_s(buffer, sizeof(buffer), "%s\np2{%f, %f}\nco2{%f,%f,%f,%f}\nr2{%f,%f}\n", this->getObjectName(otherObject).c_str(), otherObject->getPosition().x, otherObject->getPosition().y,
							otherCollidable->getPosition().x, otherCollidable->getPosition().y,
							((Square*)otherCollidable)->getMax().x, ((Square*)otherCollidable)->getMax().y,
							otherObject->getRenderable()->getPosition().x, otherObject->getRenderable()->getPosition().y);
						DEBUG_MSG(buffer);
						DEBUG_MSG("\n");
					}
#endif
				}
				checkedPairs.insert(pairHash);
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
