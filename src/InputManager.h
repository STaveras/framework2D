// File: InputManager.h
#if !defined(_INPUTMANAGER_H_)
#define _INPUTMANAGER_H_

#include "IInput.h"
#include "Factory.h"
#include "VirtualGamePad.h"
#include <vector>

class EventSystem;
class IKeyboard;

class InputManager
{
	float m_fElapsed;
	EventSystem* m_pEventSystem;
	IInput* m_pInput;

	Factory<VirtualGamePad> m_GamePads;

public:
	InputManager(void);
	~InputManager(void);

	IKeyboard* GetKeyboard(void) { return m_pInput->GetKeyboard(); }
	IMouse* GetMouse(void) { return m_pInput->GetMouse(); }
	VirtualGamePad* GetGamepad(unsigned int uiIndex) { return m_GamePads.At(uiIndex); }

	void SetEventSystem(EventSystem* pEventSystem) { m_pEventSystem = pEventSystem; }
	void SetInputInterface(IInput* pInput) { m_pInput = pInput; }

	VirtualGamePad* CreateGamePad(void);
	void DestroyGamePad(VirtualGamePad* pGamePad) { m_GamePads.Destroy(pGamePad); }

	void Initialize(EventSystem* pEventSystem, IInput* pInput) { m_pEventSystem = pEventSystem; m_pInput = pInput; }
	void Update(float fTime);
	void Shutdown(void);
};
#endif
// Author: Stanley Taveras1