// File: ObjectManager.h
#if !defined(_OBJECTMANAGER_H)
#define _OBJECTMANAGER_H

#include <list>
#include <map>
#include <string>

#include "Event.h"

#define EVENT_OBJECT_ADDED "EVT_OBJECT_ADDED"
#define EVENT_OBJECT_REMOVED "EVT_OBJECT_REMOVED"
#define EVENT_OBJ_OPERATOR_REMOVED "OBJ_OP_REMOVED"

class GameObject;
class ObjectOperator;

// Maybe should also be a factory?
class ObjectManager
{
	 std::map<std::string, GameObject*> m_mObjects;
	 std::list<ObjectOperator*> m_lsObjOperators;

public:

	std::string getObjectName(GameObject* object) {
		std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
		for (; itr != m_mObjects.end(); itr++) {
			if (itr->second == object) {
				return itr->first;
			}
		}
		return "";
	}

	void update(float fTime);

	void addObject(const char* name, GameObject* object);
	void removeObject(GameObject* object);
	void removeObject(const char* name);

	void pushOperator(ObjectOperator* objOperation);
	void popOperator(void);
	void clearOperators(void);

	size_t numObjects(void) const { return m_mObjects.size(); }

	GameObject* getGameObject(const char* name) { return m_mObjects[name]; }

	GameObject* operator[](unsigned int index) {
		std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
		for (; itr != m_mObjects.end(); itr++) {
			if (index-- == 0)
				return itr->second;
		}
		return NULL;
	}

	GameObject* operator[](const char* name) {
		return this->getGameObject(name);
	}

	void sendEvent(Event::event_key key, void* sender);
};
#endif  //_OBJECTMANAGER_H
// Author: Stanley Taveras
