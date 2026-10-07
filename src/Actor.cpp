#include "Actor.h"

#include "Controller.h"

void Actor::possess(Controller* controller)
{
	_controller = controller;
	_intent.clear();
}

void Actor::update(float time)
{
	if (_controller) {
		_controller->updateIntent(*this, time, _intent);
	}
	else {
		_intent.clear();
	}

	onUpdate(time);
}
