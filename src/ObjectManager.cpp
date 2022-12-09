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

   std::list<std::pair<GameObject*, GameObject*>> objectPairs;

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
         if ((*itr2)->isEnabled() && !(**itr2)(object))
            removalList.push_back((*itr2));
      }

      // Remove operator(s)
      while (!removalList.empty()) 
      {
         ObjectOperator *objOp = removalList.front();
         m_lsObjOperators.remove(objOp);
         removalList.pop_front();

         // TODO: Is this used somewhere?
         Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOp);
      }
      //////////////////////////////////////////////////
      // Object Operators End
      // --

      Collidable* collidable = object->getCollidable();
      if (collidable) {
         // TODO: Rewrite to broad / narrow phases
         std::map<std::string, GameObject*>::iterator itr_two = m_mObjects.begin();
         for (; itr_two != m_mObjects.end(); itr_two++) {
				if (itr_two->second == object ||
                itr->second->getType() == itr_two->second->getType() && 
                itr_two->second->getType() == GameObject::GAME_OBJ_TILE)
					continue;

            GameObject* otherObject = itr_two->second;

            bool alreadyChecked = false;

            //std::list<std::pair<GameObject*, GameObject*>>::iterator pairItr = objectPairs.begin();
            //for (; pairItr != objectPairs.end(); pairItr++) {
            //   if (pairItr->first == object && pairItr->second == otherObject ||
            //      pairItr->first == otherObject && pairItr->second == object) {
            //      alreadyChecked = true;
            //   }
            //}

            if (!alreadyChecked) {

               if (otherObject->getCollidable()) {
                  if (collidable->collidesWith(otherObject->getCollidable())) {
                     Engine2D::getEventSystem()->sendEvent(CollisionEvent(object, otherObject));
                  }
               }

               //objectPairs.push_back(std::make_pair(object, otherObject));
            }
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

            object->finish();

            m_mObjects.erase(itr);

            Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object);
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
            
            Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object);
         }
      }
   }
}

void ObjectManager::addObject(const char* name, GameObject* object)
{
   m_mObjects[name] = object; object->start();
   
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
   m_lsObjOperators.pop_back();
}

void ObjectManager::clearOperators(void)
{
   m_lsObjOperators.clear();
}

void ObjectManager::sendEvent(Event::event_key key, void* sender)
{
   std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

   for (; itr != m_mObjects.end(); itr++) {
      itr->second->sendInput(key, sender);
   }
}