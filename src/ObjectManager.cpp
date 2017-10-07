// File: ObjectManager.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "ObjectManager.h"
#include "GameObject.h"
#include "ObjectOperator.h"

void ObjectManager::Update(float fTime)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

	for(; itr != m_mObjects.end(); itr++)
	{
		// This is to apply operators on an object
		std::list<ObjectOperator*>::iterator itr2 = m_lsObjOperators.begin();

		for(; itr2 != m_lsObjOperators.end(); itr2++)
			if(!(**itr2)(itr->second))
				continue;

		itr->second->Update(fTime);
	}
}

void ObjectManager::RemoveObject(const char* name)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
	for(; itr != m_mObjects.end(); itr++)
	{
		if (itr->first == name)
		{
			m_mObjects.erase(itr);
			return;
		}
	}
}

void ObjectManager::AddObject(const char* name, GameObject* object)
{
	m_mObjects[name] = object;
}

void ObjectManager::PushOperator(ObjectOperator* objOperation)
{
	m_lsObjOperators.push_back(objOperation);
}

void ObjectManager::PopOperator(void)
{
	m_lsObjOperators.pop_back();
}

void ObjectManager::SendEvent(Event::event_key key, void* sender)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

	for(; itr != m_mObjects.end(); itr++)
	{
		itr->second->SendInput(key,sender);
	}
}