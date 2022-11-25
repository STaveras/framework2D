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

// How we should start thinking about events:
// SYSTEM EVENTS: The backend, mechanical stuff that glues the ''engine'' together (out/in)
// SIMULATION EVENTS: Pertinent only to the objects, and their interactions with one another. (in/out)

void ObjectManager::update(float fTime)
{
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

   for (; itr != m_mObjects.end(); itr++)
   {
      GameObject *object = itr->second;

      // NOTE: You should probably be sending nearly ALL events to objects

      object->update(fTime);

      // TODO: Rethink Object Operators?
      //////////////////////////////////////////////////
      // For operators that only do so once...?
      std::list<ObjectOperator*> removalList;

      // This is to apply operators on an object
      std::list<ObjectOperator*>::iterator itr2 = m_lsObjOperators.begin();
    
      for (; itr2 != m_lsObjOperators.end(); itr2++) {
         if (!(**itr2)(object))
            removalList.push_back((*itr2));
      }

      // Remove operator(s)
      while (!removalList.empty()) 
      {
         ObjectOperator *objOp = removalList.front();
         m_lsObjOperators.remove(objOp);
         removalList.pop_front();

         // TODO: Is this used somewhere?
         Engine2D::getEventSystem()->sendEvent(EVENT_OBJ_OPERATOR_REMOVED, objOp);
      }
      //////////////////////////////////////////////////
      // Object Operators End
      // --

      // Now lets check for collisions
      std::map<std::string, GameObject*>::iterator itr_two = m_mObjects.begin();
      for (; itr_two != m_mObjects.end(); itr_two++) {
         if (itr->second == object)
            continue;

         ObjectState *objectState = (ObjectState*)object->getState();

         if (objectState) {

            Collidable *collisionObject = objectState->getCollidable();

            // TODO: We need to make checking collisions happen only once per pair of objects;
            //       e.g. objA & objB will come up again, as objB & objA; the order of the two shouldn't matter

            if (collisionObject) {

               ObjectState *secondState = (ObjectState*)itr->second->getState();

               if (secondState) {

                  Collidable *otherObject = secondState->getCollidable();

                  if (otherObject && collisionObject->collidesWith(otherObject))
                     Engine2D::getEventSystem()->sendEvent(CollisionEvent(object, itr->second));
               }
            }
         }
         
         // TODO: Rewrite to broad / narrow phases
      }
   }
}

void ObjectManager::removeObject(GameObject* object)
{
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
   for (; itr != m_mObjects.end(); itr++) {
      if (itr->second == object) {
         if (object) {

            object->finish();

            m_mObjects.erase(itr);

            Engine2D::getEventSystem()->sendEvent(EVENT_OBJECT_REMOVED, object);
         }
      }
   }
}

void ObjectManager::removeObject(const char* name)
{
   GameObject *object = NULL;
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
   for (; itr != m_mObjects.end(); itr++) {
      if (itr->first == name) {
         object = itr->second;

         if (object)
         {
            object->finish();

            m_mObjects.erase(itr);
            
            Engine2D::getEventSystem()->sendEvent(EVENT_OBJECT_REMOVED, object);
         }
      }
   }
}

void ObjectManager::addObject(const char* name, GameObject* object)
{
   m_mObjects[name] = object; object->start();
   
   Engine2D::getEventSystem()->sendEvent(EVENT_OBJECT_ADDED, object);
}

// NOTE: Maybe use a map instead and add/remove the operators by name?

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

   for (; itr != m_mObjects.end(); itr++) {
      itr->second->sendInput(key, sender);
   }
}