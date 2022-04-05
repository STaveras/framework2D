// File: ObjectManager.h
#if !defined(_OBJECTMANAGER_H)
#define _OBJECTMANAGER_H

#include <list>
#include <map>
#include <string>

#include "Event.h"

class GameObject;
class ObjectOperator;

// Maybe should also be a factory?
class ObjectManager
{
	 std::map<std::string, GameObject*> m_mObjects;
	 std::list<ObjectOperator*> m_lsObjOperators;

public:
	 GameObject* getGameObject(const char* name) { return m_mObjects[name]; }

	 void Update(float fTime);

	 void addObject(const char* name, GameObject* object);
	 void removeObject(const char* name);

	 void pushOperator(ObjectOperator* objOperation);
	 void popOperator(void);
	 void clearOperators(void);

	 void SendEvent(Event::event_key key, void* sender);
};
#endif  //_OBJECTMANAGER_H
// Author: Stanley Taveras
