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

   std::string getObjectName(GameObject *object) {
      std::map<std::string, GameObject *>::iterator itr = m_mObjects.begin();
      for (; itr != m_mObjects.end(); itr++) {
         if (itr->second == object) {
            return itr->first;
         }
      }
      return "";
   }

   GameObject* getGameObject(const char* name) { return m_mObjects[name]; }

	 void Update(float fTime);

	 void addObject(const char* name, GameObject* object);
	 void removeObject(const char* name);

	 void pushOperator(ObjectOperator* objOperation);
	 void popOperator(void);
	 void clearOperators(void);

   size_t numObjects(void) const { return m_mObjects.size(); }

   GameObject* operator[](unsigned int index) { 
      std::map<std::string, GameObject *>::iterator itr = m_mObjects.begin();
      for (; itr != m_mObjects.end(); itr++) {
         if (index-- == 0)
            return itr->second;
      }
      return NULL;
   }

   void SendEvent(Event::event_key key, void* sender);
};
#endif  //_OBJECTMANAGER_H
// Author: Stanley Taveras
