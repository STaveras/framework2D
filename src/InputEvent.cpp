
#include "InputEvent.h"

InputEvent::InputEvent(Event::event_key evtKey, void* pSender, Controller* controller, float time, std::string actionName) :
	Event(evtKey, pSender, Event::event_priority_high),
	_timeStamp(time),
	_controller(controller),
	_actionName(actionName) {

}