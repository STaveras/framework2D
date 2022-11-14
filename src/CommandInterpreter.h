
#pragma once

#include "Cyclable.h"
#include "Command.h"
#include "Event.h"
#include "Factory.h"
#include "Action.h"
#include <queue>
#include <vector>

#define COMMAND_PERIOD 2
#define COMMAND_SIMULTANEOUS 1
#define COMMAND_LONGPERIOD COMMAND_PERIOD * 2
#define COMMAND_RESETTIME COMMAND_LONGPERIOD + 1

class EventSystem;
class Controller;

class CommandInterpreter: public Cyclable
{
	float m_fResetTimer;
	EventSystem* m_pEventSystem;
	Factory<Command> m_Commands;

	std::vector<Action> m_vButtons;
	std::vector<Action> m_vHeldButtons;
	std::map<ActionName, float> m_mHoldTimes;

protected:
	bool _ProcessCommand(char* szCommand);
	float _ParseHoldTime(char* pCmdIter);
	char* _NextCommand(char* pCmdIter);
	Action* _ButtonInBuffer(char* szButtonID, size_t dwStartIndex);

public:
	CommandInterpreter(void);

	void AddCommand(const char* szCommand);
	void RemoveCommand(const char* szCommand);

	void RegisterKeyPress(ActionName btnID, float fTimeStamp);
	void RegisterKeyRelease(ActionName btnID, float fTimeStamp);

	void start(EventSystem* pEventSystem, Controller* pGamePad);
	void update(float fTime);
	void finish(void);
};