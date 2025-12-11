// File: Controller.h
#pragma once

#include "IInput.h"
#include "Action.h"
#include "Cyclable.h"
#include "Event.h"
#include "Engine2D.h"

#include <list>

class Controller
{
public:
	Controller(void) :
		_connected(false),
		_padNumber(-1),
		_elapsedTime(0.0f),
		_input(NULL),
		_eventSystem(NULL) {
	}
	~Controller(void) {}

	bool isConnected(void) const { return _connected; }
	int getPadNumber(void) const { return _padNumber; }

	IInput* getInputInterface(void) { return _input; }
	void setInputInterface(IInput* pInput) { _input = pInput; }

	EventSystem* getEventSystem(void) { return _eventSystem; }
	void setEventSystem(EventSystem* eventSystem) { _eventSystem = eventSystem; }

	void setIsConnected(bool connected) { _connected = connected; }
	void setPadNumber(int padNumber) { _padNumber = padNumber; }

	void addAction(Action action) { _actions.push_back(action); }
	void removeAction(Action action);

	bool buttonPressed(Action* action);
	bool buttonReleased(Action* action);
	bool buttonDown(Action* action);
	bool buttonUp(Action* action);

	void update(float time);

	Action* getAction(std::string actionName);
	std::list<Action>& getActions(void) { return _actions; }

private:
	bool _connected;
	int _padNumber;
	float _elapsedTime;
	InputInterface* _input;
	EventSystem* _eventSystem;

	std::list<Action> _actions;

public:
	class EventListener : public Cyclable
	{
	public:
		virtual void onButtonDown(const Event& evt) = 0;
		virtual void onButtonUp(const Event& evt) = 0;
		virtual void onButtonPressed(const Event& evt) = 0;
		virtual void onButtonReleased(const Event& evt) = 0;
		virtual ~EventListener() {}

		virtual void start(void) {
			Engine2D::getInstance()->getEventSystem()->registerCallback<EventListener>(EVT_KEYPRESSED, this, &EventListener::onButtonPressed);
			Engine2D::getInstance()->getEventSystem()->registerCallback<EventListener>(EVT_KEYRELEASED, this, &EventListener::onButtonReleased);
			Engine2D::getInstance()->getEventSystem()->registerCallback<EventListener>(EVT_KEYDOWN, this, &EventListener::onButtonDown);
			Engine2D::getInstance()->getEventSystem()->registerCallback<EventListener>(EVT_KEYUP, this, &EventListener::onButtonUp);
		}

		virtual void update(float time) {
			throw std::runtime_error("EventListener::update() not implemented.");
		}

		virtual void finish(void) {
			Engine2D::getInstance()->getEventSystem()->unregister<EventListener>(EVT_KEYUP, this, &EventListener::onButtonUp);
			Engine2D::getInstance()->getEventSystem()->unregister<EventListener>(EVT_KEYDOWN, this, &EventListener::onButtonDown);
			Engine2D::getInstance()->getEventSystem()->unregister<EventListener>(EVT_KEYRELEASED, this, &EventListener::onButtonReleased);
			Engine2D::getInstance()->getEventSystem()->unregister<EventListener>(EVT_KEYPRESSED, this, &EventListener::onButtonPressed);
		}
	};
};

// Author: Stanley Taveras