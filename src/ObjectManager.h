// File: ObjectManager.h
#if !defined(_OBJECTMANAGER_H)
#define _OBJECTMANAGER_H

#include <list>
#include <map>
#include <string>

#include "Event.h"

class GameObject;
class ObjectOperator;

class ObjectManager
{
	std::map<std::string, GameObject*> m_mObjects;
	std::list<ObjectOperator*> m_lsObjOperators;

public:
	GameObject* GetObject(const char* name) { return m_mObjects[name]; }

	void Update(float fTime);

	void AddObject(const char* name, GameObject* object);
	void RemoveObject(const char* name);

	void PushOperator(ObjectOperator* objOperation);
	void PopOperator(void);

	void SendEvent(Event::event_key key, void* sender);
};
#endif  //_OBJECTMANAGER_H
// Author: Stanley Taveras
