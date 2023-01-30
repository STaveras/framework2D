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

// How we should start thinking about events:
// SYSTEM EVENTS: The backend, mechanical stuff that glues the ''engine'' together (out/in)
// SIMULATION EVENTS: Pertinent only to the objects, and their interactions with one another. (in/out)

void ObjectManager::update(float fTime)
{
   std::map<std::string, GameObject*>::iterator objectItr = m_mObjects.begin();

   std::list<std::pair<GameObject*, GameObject*>> objectPairs;

   for (; objectItr != m_mObjects.end(); objectItr++)
   {
      GameObject *object = objectItr->second;
      object->update(fTime);

      // I really did not want to have any sort of object-specific checking inside the object manager -- I wanted it to be completely oblivious to whatever objects it's holding
      if (objectItr->second->getType() == GameObject::GAME_OBJ_TILE)
         continue;

      // TODO: Rethink Object Operators?
      //////////////////////////////////////////////////
      // For operators that only do so once...?
      std::list<ObjectOperator*> removalList;

      // This is to apply operators on an object
      std::list<ObjectOperator*>::iterator operatorItr = m_lsObjOperators.begin();

      for (; operatorItr != m_lsObjOperators.end(); operatorItr++) {
         if ((*operatorItr)->isEnabled() && !(**operatorItr)(object))
            removalList.push_back((*operatorItr));
      }

      // Remove operator(s)
      while (!removalList.empty())
      {
         ObjectOperator* objOp = removalList.front();
         m_lsObjOperators.remove(objOp);
         removalList.pop_front();

         // TODO: Is this used somewhere?
         Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOp);
      }
      //////////////////////////////////////////////////
      // Object Operators End
      // --

      // TODO: Rewrite to broad / narrow phases
      if (Collidable* collidable = object->getCollidable()) {

         std::map<std::string, GameObject*>::iterator otherObjectItr = m_mObjects.begin();

         for (; otherObjectItr != m_mObjects.end(); otherObjectItr++) {

            if (otherObjectItr->second == object ||
               objectItr->second->getType() == otherObjectItr->second->getType() &&
               objectItr->second->getType() == GameObject::GAME_OBJ_TILE) {
               continue;
            }

            GameObject* otherObject = otherObjectItr->second;

            bool alreadyChecked = false;

            //std::list<std::pair<GameObject*, GameObject*>>::iterator pairItr = objectPairs.begin();
            //for (; pairItr != objectPairs.end(); pairItr++) {
            //   if (pairItr->first == object && pairItr->second == otherObject ||
            //      pairItr->first == otherObject && pairItr->second == object) {
            //      alreadyChecked = true;
            //   }
            //}

            if (!alreadyChecked) {

               if (Collidable* otherCollidable = otherObject->getCollidable()) {

                  if (collidable->collidesWith(otherCollidable)) {
#if _DEBUG
                     if (Debug::dbgCollision) {
                        char buffer[256]{ 0 };
                        sprintf_s(buffer, "%s\np1{%f, %f}\nco1{%f,%f,%f,%f}\nr1{%f,%f}\n", objectItr->first.c_str(), object->getPosition().x, object->getPosition().y,
                                                                      collidable->getPosition().x, collidable->getPosition().y,
                                                            ((Square*)collidable)->getMax().x, ((Square*)collidable)->getMax().y,
                                                                      object->getRenderable()->getPosition().x, object->getRenderable()->getPosition().y);
                        DEBUG_MSG(buffer);
                        DEBUG_MSG("+\n")

                        sprintf_s(buffer, "%s\np2{%f, %f}\nco2{%f,%f,%f,%f}\nr2{%f,%f}\n", otherObjectItr->first.c_str(), otherObject->getPosition().x, otherObject->getPosition().y,
                                                                     otherCollidable->getPosition().x, otherCollidable->getPosition().y,
                                                           ((Square*)otherCollidable)->getMax().x, ((Square*)otherCollidable)->getMax().y, 
                                                                     otherObject->getRenderable()->getPosition().x, otherObject->getRenderable()->getPosition().y);

                        DEBUG_MSG(buffer);
                        DEBUG_MSG("\n");
                     }
#endif
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

            break;
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
