
#include "CommandInterpreter.h"
#include "CommandEvent.h"
#include "EventSystem.h"
#include "InputEvent.h"
#include "Controller.h"

CommandInterpreter::CommandInterpreter(void):
m_fResetTimer(0.0f),
_eventSystem(NULL)
{}

bool CommandInterpreter::_ProcessCommand(char* szCommand)
{
	size_t dwBtnIndex = 0;
	Action* pPrevAction = NULL;
	char* pCmdItr = szCommand;

	while(pCmdItr)
	{
		bool bPass = false;
		Action* pAction = NULL;

		switch(*pCmdItr)
		{
		case '.': // Short tap
			{
				pCmdItr++;

				char* szButton = _NextCommand(pCmdItr);

				if(szButton)
				{
					bool bSkipped = false;

					for(; dwBtnIndex < m_vButtons.size(); dwBtnIndex++)
					{
						pAction = &m_vButtons[dwBtnIndex];

						if(!strcmp(szButton, pAction->getActionName().c_str()))
						{
							if(pPrevAction->getActionName() == pAction->getActionName() && !bSkipped)
							{
								bSkipped = true;
								continue;
							}
							else if(fabsf(pAction->getActionTime() - pPrevAction->getActionTime()) <= COMMAND_PERIOD)
							{
								bPass = true;
								dwBtnIndex++;
								break;
							}
						}
					}

					delete [] szButton;
				}
			}
			break;

		case ',':
			{
				pCmdItr++;

				char* szButton = _NextCommand(pCmdItr);

				if(szButton)
				{
					bool bSkipped = false;

					for(; dwBtnIndex < m_vButtons.size(); dwBtnIndex++)
					{
						pAction = &m_vButtons[dwBtnIndex];

						if(!strcmp(szButton, pAction->getActionName().c_str()))
						{
							if(pPrevAction->getActionName() == pAction->getActionName() && !bSkipped)
							{
								bSkipped = true;
								continue;
							}
							else if(fabsf(pAction->getActionTime() - pPrevAction->getActionTime()) <= COMMAND_LONGPERIOD)
							{
								bPass = true;
								dwBtnIndex++;
								break;
							}
						}
					}

					delete [] szButton;
				}
			}
			break;

		case '+': // Simultaneous
			{
				pCmdItr++;

				char* szButton = _NextCommand(pCmdItr);

				if(szButton)
				{
					pAction = _ButtonInBuffer(szButton, dwBtnIndex);

					if(pAction && (!pPrevAction || pAction->simultaneous(*pPrevAction))) //fabsf(pAction->getTimeStamp() - pPrevAction->getTimeStamp()) <= COMMAND_SIMULTANEOUS
					{
						bPass = true;
						dwBtnIndex++;
					}

					delete [] szButton;
				}
			}
			break;

		default:
			{
				char* szButton = _NextCommand(pCmdItr);

				if(szButton)
				{
					if(*pCmdItr == '|')
					{
						float fTime = _ParseHoldTime(pCmdItr);
		
						if(m_mHoldTimes[szButton] >= fTime)
						{
							bPass = true;
						}
					}
					else
					{
						pAction = _ButtonInBuffer(szButton, dwBtnIndex);

						if(pAction)
						{
							bPass = true;
							dwBtnIndex++;
						}
					}

					delete [] szButton;
				}
			}
			break;
		} // switch

		if(!bPass)
			break;

		if(pPrevAction != pAction)
			pPrevAction = pAction;
	} // while

	return true;
}

float CommandInterpreter::_ParseHoldTime(char* pCmdIter)
{
	std::string szNum = "";
	
	while(!isalpha(*pCmdIter) && pCmdIter && *pCmdIter != '-')
	{
		szNum += (*pCmdIter);
		pCmdIter++;
	}

	return (float)atof(szNum.c_str());
}

char* CommandInterpreter::_NextCommand(char* pCmdIter)
{
	char szDelims[] = "+.,|- \0";
	
	size_t dwIndex = strcspn(pCmdIter, szDelims);

	if(dwIndex == 0)
		return NULL;

	char* szReturn = new char[dwIndex+1];
	memset(szReturn, 0, dwIndex+1);
	strncpy_s(szReturn, sizeof(char) * (dwIndex+1), pCmdIter, dwIndex);

	pCmdIter += dwIndex;

	return szReturn;
}

Action* CommandInterpreter::_ButtonInBuffer(char* szButtonID, size_t dwStartIndex)
{
	for (int i = (int)dwStartIndex; i < (int)m_vButtons.size(); i++)
	{
		if(!strcmp(szButtonID, m_vButtons[i].getActionName().c_str())) {
			return &m_vButtons[i];
		}
	}

	return NULL;
}

void CommandInterpreter::AddCommand(const char* command)
{
	m_Commands.create(Command(command));
	m_Commands.sort(Command::less());
}

void CommandInterpreter::RemoveCommand(const char* szCommand)
{
	//m_Commands.destroy(m_Commands.find(Command(command)));
}

void CommandInterpreter::RegisterKeyPress(std::string actionName, float time)
{
	m_fResetTimer = 0;

	bool bExists = false;
	Action btn;

	for(size_t i = 0; i < m_vHeldButtons.size(); i++)
	{
		if(actionName == m_vHeldButtons[i].getActionName())
		{
			bExists = true;
			m_vHeldButtons[i].setActionTime(time);
			continue;
		}

		btn = Action(m_vHeldButtons[i].getActionName());
		btn.setActionTime(time);
		m_vButtons.push_back(btn);
	}

	if(!bExists)
	{
		m_vHeldButtons.push_back(btn);
	}

	btn = Action(actionName);
	btn.setActionTime(time);
	m_vButtons.push_back(btn);

	m_mHoldTimes[actionName] = 0;
}

void CommandInterpreter::RegisterKeyRelease(std::string actionName, float time)
{
	m_fResetTimer = 0;

	for(std::vector<Action>::iterator itr = m_vHeldButtons.begin(); itr != m_vHeldButtons.end(); itr++)
	{
		if(actionName == itr->getActionName())
		{
			m_mHoldTimes[actionName] = time - itr->getActionTime();
			m_vHeldButtons.erase(itr);
			break;
		}
	}

	Action btn(actionName);
	btn.setActionTime(time);

	m_vButtons.push_back(btn);
}

void CommandInterpreter::start(EventSystem* pEventSystem, Controller* pGamePad)
{
	_eventSystem = pEventSystem;
}

void CommandInterpreter::update(float fTime)
{
	m_fResetTimer += fTime;

	Factory<Command>::const_factory_iterator command_iter = m_Commands.begin();

	for(; command_iter != m_Commands.end(); command_iter++)
	{
		if(m_vButtons.size() + m_vHeldButtons.size() < (*command_iter)->Length())
			break;

		if(_ProcessCommand((*command_iter)->GetCommandString()))
		{
			(*command_iter)->SetTimeStamp(m_vButtons[m_vButtons.size()-1].getActionTime());
			_eventSystem->sendEvent<CommandEvent>(CommandEvent((*command_iter), this));
		}
	}

	// Update held buttons as necessary; reset the ones that aren't
	for (std::map<std::string, float>::iterator itr = m_mHoldTimes.begin(); itr != m_mHoldTimes.end(); itr++)
	{
		bool bAddTime = false;

		for (size_t i = 0; i < m_vHeldButtons.size(); itr++)
		{
			if (itr->first == m_vHeldButtons[i].getActionName())
			{
				bAddTime = true;
				break;
			}
		}

		if (bAddTime) {
			itr->second += fTime;
		}
		else {
			itr->second = 0;
		}
	}

	// If buttons haven't been pressed for a while, clear the button buffer
	if(m_fResetTimer >= COMMAND_RESETTIME)
	{
		m_fResetTimer = 0.0f;
		m_vButtons.clear();
	}
}

void CommandInterpreter::finish(void)
{
	m_vHeldButtons.clear();
	m_vButtons.clear();
	m_Commands.clear();
}