// File: Collidable.h
#pragma once
#include "COL_OBJ_TYPE.h"
#include "Positionable.h"
#include "Types.h"
#include "Event.h"

class GameObject;

class Collidable: public Positionable
{
   COL_OBJ_TYPE m_eType;

protected:
   Collidable(COL_OBJ_TYPE type) :m_eType(type) {}

public:
   COL_OBJ_TYPE getType(void) const { return m_eType; } 

   virtual bool Check(const Collidable* colObject) = 0;
};

// NOTE: Would be nice to have a 'batched' version of this
class CollisionEvent: public Event
{
   GameObject* involvedObject;

public:
   CollisionEvent(GameObject *object, GameObject *collidedWith) :Event("EVT_COLLISION", object), involvedObject(collidedWith) {}
};
// Author: Stanley Taveras