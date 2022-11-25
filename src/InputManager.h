// File: InputManager.h
#if !defined(_INPUTMANAGER_H_)
#define _INPUTMANAGER_H_

#include "IInput.h"
#include "Factory.h"
#include "Controller.h"
#include <vector>

class EventSystem;
class IKeyboard;

// This is confusing at the moment because originally "VirtualGamePad" was just that... A virtual representation of a game pad or console controller...
// But what a controller is now is an action-input mapper and the inputmanager updates both controllers and input devices
// This allows controllers to eventually take input from other sources, such as non-human agents (machine/AI)

class InputManager
{
	float m_fElapsed;
	EventSystem* m_pEventSystem;
	IInput* _input;

	Factory<Controller> m_Controllers;

public:
	InputManager(void);
	~InputManager(void);

	IKeyboard* getKeyboard(void) { return _input->getKeyboard(); }
	IMouse* getMouse(void) { return _input->getMouse(); }

	Controller* getController(unsigned int uiIndex) { return m_Controllers.at(uiIndex); }

	void setEventSystem(EventSystem* pEventSystem) { m_pEventSystem = pEventSystem; }
	void setInputInterface(IInput* pInput) { _input = pInput; }

	Controller* createController(void);
	void destroyController(Controller* controller) { m_Controllers.destroy(controller); }

	void initialize(EventSystem* pEventSystem, IInput* pInput) { m_pEventSystem = pEventSystem; _input = pInput; }
	void update(float fTime);
	void shutdown(void);
};
#endif
// Author: Stanley Taveras1