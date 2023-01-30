// CollisionEvent.h

#pragma once

//#include "GameObject.h"

class GameObject;

#ifndef _COLLISIONEVENT_H_
#define _COLLISIONEVENT_H_

#define EVT_COLLISION "EVT_COLLISION"

// NOTE: Would be nice to have a 'batched' version of this
struct CollisionEvent : public Event
{
	GameObject* involvedObject;

	// "object" is the sender
	CollisionEvent(GameObject* object, GameObject* collidedWith) :
		Event(EVT_COLLISION, object),
		involvedObject(collidedWith) {
		//      object->setIsColliding(true);
		//collidedWith->setIsColliding(true);
	}
};

#endif
