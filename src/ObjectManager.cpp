// File: ObjectManager.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "ObjectManager.h"
#include "GameObject.h"
#include "ObjectOperator.h"

typedef GameObject::GameObjectState ObjectState;

void ObjectManager::Update(float fTime)
{
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

   for (; itr != m_mObjects.end(); itr++)
   {
      GameObject *object = itr->second;

      // This is to apply operators on an object
      std::list<ObjectOperator*>::iterator itr2 = m_lsObjOperators.begin();
    
      for (; itr2 != m_lsObjOperators.end(); itr2++) {
         if (!(**itr2)(object))
            continue;
         else {
            // Remove the operator
         }
      }

      object->update(fTime);
   }
}

void ObjectManager::removeObject(const char* name)
{
   GameObject *object = NULL;
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
   for (; itr != m_mObjects.end(); itr++) {
      if (itr->first == name) {
         object = itr->second; ((GameObject*)itr->second)->Shutdown(); m_mObjects.erase(itr); return;
      }
   }
   if (object)
      Engine2D::getEventSystem()->sendEvent("EVT_OBJECT_REMOVED", object);
}

void ObjectManager::addObject(const char* name, GameObject* object)
{
   m_mObjects[name] = object; object->Setup(); Engine2D::getEventSystem()->sendEvent("EVT_OBJECT_ADDED", object);
}

void ObjectManager::pushOperator(ObjectOperator* objOperation)
{
   m_lsObjOperators.push_back(objOperation);
}

void ObjectManager::popOperator(void)
{
   m_lsObjOperators.pop_back();
}

void ObjectManager::clearOperators(void)
{
   m_lsObjOperators.clear();
}

void ObjectManager::SendEvent(Event::event_key key, void* sender)
{
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

   for (; itr != m_mObjects.end(); itr++)
   {
      itr->second->SendInput(key, sender);
   }
}