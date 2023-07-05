#pragma once

#include "ObjectOperator.h"
#include "GameObject.h"
#include "Event.h"
#include "Timer.h"
#include "Maths.h"

#define EVT_FORCE_EVENT "EVT_FORCE_EVENT"

class ForceEvent : public Event
{
	struct ForceOperator* _operator;

public: 
	ForceEvent(struct ForceOperator* objectOperator) : Event(EVT_FORCE_EVENT, objectOperator), _operator(objectOperator) {}
	// Add more information...?
};

struct ForceOperator : public ObjectOperator
{
	Timer _timer;

	float	_force;
	float _duration; // if it's a temporarily applied force
	vector2 _direction;

	bool operator()(GameObject* object) {

		_timer.update();

		object->addImpulse(_direction, _force);

	}
};
