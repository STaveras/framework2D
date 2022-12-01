// File: Collidable.h
#pragma once
#include "COL_OBJ_TYPE.h"
#include "Positionable.h"
#include "Types.h"
#include "Event.h"

class GameObject;

class Collidable : public Positionable
{
	Collidable(void) :Positionable(), m_eType(COL_OBJ_VOID) {}

public:
	enum COL_OBJ_TYPE
	{
		COL_OBJ_VOID,
		COL_OBJ_CIRCLE,
		COL_OBJ_PLANE,
		COL_OBJ_SQUARE,
		COL_OBJ_GROUP
	};

private:
	COL_OBJ_TYPE m_eType;

protected:

	explicit Collidable(COL_OBJ_TYPE type) : Positionable(), m_eType(type) {}

	virtual ~Collidable(void) = default; // GCC/Clang studidity

public:

	virtual bool collidesWith(vector2 point) { return _position == point; } // issa joke
	virtual bool collidesWith(const Collidable* collidable) { return this->collidesWith(collidable->getPosition()); }

	COL_OBJ_TYPE getType(void) const { return m_eType; }

};

#define EVENT_COLLISION "EVT_COLLISION"

// NOTE: Would be nice to have a 'batched' version of this
struct CollisionEvent : public Event
{
	GameObject* involvedObject;

	// "object" is the sender
	CollisionEvent(GameObject* object, GameObject* collidedWith) :Event(EVENT_COLLISION, object), involvedObject(collidedWith) {}
};
// Author: Stanley Taveras