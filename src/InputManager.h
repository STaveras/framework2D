// File: InputManager.h
#if !defined(_INPUTMANAGER_H_)
#define _INPUTMANAGER_H_

#include "IInput.h"
#include "Factory.h"
#include "InputMap.h"
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
	Factory<InputMap> _inputMaps;

public:
	InputManager(void);
	~InputManager(void);

	void initialize(EventSystem* eventSystem, IInput* inputInterface);

	InputMap* createInputMap(void);
	InputMap* getInputMap(unsigned int uiIndex) { return _inputMaps.at(uiIndex); }
	void destroyInputMap(InputMap* controller);

	Keyboard* getKeyboard(void) { return _input ? _input->getKeyboard() : NULL; }
	Mouse*	 getMouse(void)	 { return _input ? _input->getMouse() : NULL; }
	Gamepad* getGamepad(void) { return _input ? _input->getGamepad() : NULL; }
	
	void update(float fTime);
	void shutdown(void);
};
#endif
// Author: Stanley Taveras
