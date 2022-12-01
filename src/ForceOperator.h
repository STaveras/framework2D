#pragma once

#include "ObjectOperator.h"
#include "Event.h"
#include "Timer.h"

#define EVT_FORCE_EVENT "EVT_FORCE_EVENT"

class ForceEvent : public Event
{
	Timer _timer;
	struct ForceOperator* _operator;

public: 
	ForceEvent(struct ForceOperator* objectOperator) : Event(EVT_FORCE_EVENT, objectOperator), _operator(objectOperator) {}
	// Add more information...?
};

struct ForceOperator : public ObjectOperator
{
	float	_force;
	vector2 _direction;

	bool operator()(GameObject* object) {

		return object->sendInput(ForceEvent(this));

	}
};
