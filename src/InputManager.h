// File: InputManager.h
#if !defined(_INPUTMANAGER_H_)
#define _INPUTMANAGER_H_

#include "IInput.h"
#include "Factory.h"
#include "Controller.h"
#include <vector>

class EventSystem;

// This is confusing at the moment because originally "VirtualGamePad" was just that... A virtual representation of a game pad or console controller...
// But what a controller is now is an action-input mapper and the inputmanager updates both controllers and input devices
// This allows controllers to eventually take input from other sources, such as non-human agents (machine/AI)

class InputManager
{
protected:
	EventSystem* _eventSystem;
	InputInterface* _input;

private:
	Factory<Controller> _controllers;

public:
	InputManager(void);
	~InputManager(void);

	void initialize(EventSystem* eventSystem, IInput* inputInterface);

	Controller* createController(void);
	Controller* getController(unsigned int uiIndex) { return _controllers.at(uiIndex); }
	void destroyController(Controller* controller);

	Keyboard* getKeyboard(void) { return _input->getKeyboard(); }
	Mouse*	 getMouse(void)	 { return _input->getMouse(); }
	
	void update(float fTime);
	void shutdown(void);
};
#endif
// Author: Stanley Taveras1