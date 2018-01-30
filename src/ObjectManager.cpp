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
      ObjectState* objectState = (ObjectState*)object->GetCurrentState();

      // This is to apply operators on an object
      std::list<ObjectOperator*>::iterator itr2 = m_lsObjOperators.begin();

      // NOTE: Should there be an operator that sets the renderable's location...?
      for (; itr2 != m_lsObjOperators.end(); itr2++)
         if (!(**itr2)(object))
            continue;

      // This is to apply the object's state changes to it
      object->AddImpulse(objectState->getDirection(), objectState->getForce(), fTime);
      object->Update(fTime);
   }
}

void ObjectManager::RemoveObject(const char* name)
{
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
   for (; itr != m_mObjects.end(); itr++) {
      if (itr->first == name) {
         ((GameObject*)itr->second)->Shutdown(); m_mObjects.erase(itr); return;
      }
   }
}

void ObjectManager::AddObject(const char* name, GameObject* object)
{
   m_mObjects[name] = object; object->Setup();
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

   for (; itr != m_mObjects.end(); itr++)
   {
      itr->second->SendInput(key, sender);
   }
}