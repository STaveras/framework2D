
#pragma once

#include "Command.h"
#include "Event.h"
#include "Factory.h"
#include "VirtualButton.h"
#include <queue>
#include <vector>

#define COMMAND_PERIOD 2
#define COMMAND_SIMULTANEOUS 1
#define COMMAND_LONGPERIOD COMMAND_PERIOD * 2
#define COMMAND_RESETTIME COMMAND_LONGPERIOD + 1

class EventSystem;
class VirtualGamePad;

class CommandInterpreter
{
	float m_fResetTimer;
	EventSystem* m_pEventSystem;
	Factory<Command> m_Commands;

	std::vector<VirtualButton> m_vButtons;
	std::vector<VirtualButton> m_vHeldButtons;
	std::map<ButtonID, float> m_mHoldTimes;

protected:
	bool _ProcessCommand(char* szCommand);
	float _ParseHoldTime(char* pCmdIter);
	char* _NextCommand(char* pCmdIter);
	VirtualButton* _ButtonInBuffer(char* szButtonID, size_t dwStartIndex);

public:
	CommandInterpreter(void);

	void AddCommand(const char* szCommand);
	void RemoveCommand(const char* szCommand);

	void RegisterKeyPress(ButtonID btnID, float fTimeStamp);
	void RegisterKeyRelease(ButtonID btnID, float fTimeStamp);

	void Initialize(EventSystem* pEventSystem, VirtualGamePad* pGamePad);
	void Update(float fTime);
	void Shutdown(void);
};