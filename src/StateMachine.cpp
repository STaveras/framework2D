// File: StateMachine.cpp

// TODO : Use Lua to load the transition table instead of your pseudo-Lua-like script?

#include "StateMachine.h"
#include "State.h"
#include "StrUtil.h"

#include <fstream>

StateMachine::StateMachine(void):
m_bBuffered(false),
m_fTransitionFrequency(1.0f),
m_pStartState(NULL),
m_pState(NULL)
{}

State* StateMachine::_GetNextState(const StateMachineEvent& evt)
{
	std::pair<std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator,
		std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator> range = m_mTransitionTable.equal_range(m_pState);

	std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator itr = range.first;

	for(; itr != range.second; itr++)
	{
		if(itr->second.first == evt)
			return itr->second.second;
	}

	return NULL;
}

void StateMachine::_Transition(State* pNextState)
{
	if(pNextState)
	{
		if(m_pState)
			m_pState->OnExit();

		pNextState->OnEnter(m_pState);
		m_pState = pNextState;
	}
}

State* StateMachine::GetState(const char* szName)
{
	Factory<State>::const_factory_iterator itr = m_States.Begin();

	for(; itr != m_States.End(); itr++)
	{
		if(!strcmp(szName, (*itr)->GetName()))
			return (*itr);
	}

	return NULL;
}

State* StateMachine::AddState(const char* name)
{
	State* state = m_States.Create();
	state->SetName(name);

	return state;
}

void StateMachine::AddTransition(const char* condition, const char* nextState)
{
	RegisterTransition(m_States.At(m_States.Size()-1)->GetName(),condition,nextState);
}

void StateMachine::RegisterTransition(State* pState, const StateMachineEvent& evt, State* pStateResult) 
{ 
	m_mTransitionTable.insert(std::make_pair(pState, std::make_pair(evt, pStateResult)));
}

void StateMachine::RegisterTransition(const char* szStateName, const char* szCondition, const char* szResultState)
{
	State* pState = GetState(szStateName);
	State* pStateResult = GetState(szResultState);

	if(pState && pStateResult)
	{
		StateMachineEvent evt(szCondition, NULL);
		m_mTransitionTable.insert(std::make_pair(pState, std::make_pair(evt, pStateResult)));
	}
}

void StateMachine::Initialize(void)
{
	if(m_pStartState)
	{
		m_pState = m_pStartState;
		m_pState->OnEnter(NULL);
	}
	else if (m_States.Size() > 0)
	{
		m_pStartState = m_States.At(0);
		this->Initialize(); // Let's go again!
	}
}

void StateMachine::Reset(void)
{
	if(m_pState)
		m_pState->OnExit();

	if(m_pStartState)
		m_pState = m_pStartState;

	while(!m_qEvents.empty())
		m_qEvents.pop();
}

void StateMachine::Terminate(void)
{
	m_mTransitionTable.clear();
	m_States.Clear();
}

void StateMachine::OnEvent(const StateMachineEvent& evt)
{
	if(m_bBuffered)
		m_qEvents.push(evt);
	else
		_Transition(_GetNextState(evt));
}

void StateMachine::SendInput(const char* szCondition, void* pSender)
{
	StateMachineEvent evt(szCondition, pSender);
	this->OnEvent(evt);
}

void StateMachine::Update(float fTime)
{
	if (m_pState && !m_pState->OnExecute(fTime) && m_pStartState)
        SetState(m_pStartState);

	if (m_bBuffered && !m_qEvents.empty())
	{
		static float fTransitionTimer = 0.0f;
		fTransitionTimer += fTime;

		if(fTransitionTimer >= m_fTransitionFrequency)
		{
			_Transition(_GetNextState(m_qEvents.front()));
			m_qEvents.pop();

			fTransitionTimer = 0.0f;
		}
	}
}

bool StateMachine::LoadTransitionTableFromFile(const char* szFilename)
{
	std::ifstream ifl(szFilename);

	if(!ifl.good())
	{
		ifl.close();
		ifl.clear();

		return false;
	}
	else 
	{
		char szBuffer[256];

		while(ifl.good())
		{
			ifl.getline(szBuffer, 256);

			if(!strcmp(szBuffer, "States"))
			{
				ifl.getline(szBuffer, 256);

				if(STR_EQUALS(szBuffer, "{"))
				{
					while(true)
					{
						ifl.getline(szBuffer, 256);

						if(STR_EQUALS(szBuffer, "}"))
							break;

						char* szStateName = strtrlws(szBuffer);
						strsubst(szStateName, '\0', ",");

						if(!GetState(szStateName) && !STR_EQUALS(szStateName, ""))
							m_States.Create(State(szStateName));
						
						if(!m_pStartState && GetState(szStateName))
							m_pStartState = GetState(szStateName);
					}
				}
			} // States
			else if(!strcmp(szBuffer, "Transitions"))
			{
				ifl.getline(szBuffer, 256);
				char* szBrace = strtrlws(szBuffer);

				if(STR_EQUALS(szBrace, "{"))
				{
					while(true)
					{
						ifl.getline(szBuffer, 256);
						szBrace = strtrlws(szBuffer);

						if(STR_EQUALS(szBrace, "}"))
							break;

						std::string strStateName = strtrlws(szBuffer);

						State* pState = GetState(strStateName.c_str());

						if(pState)
						{
							ifl.getline(szBuffer, 256);							
							szBrace = strtrlws(szBuffer);

							if(STR_EQUALS(szBrace, "{"))
							{
								while(true)
								{
									ifl.getline(szBuffer, 256);
									szBrace = strtrlws(szBuffer);

									if(STR_EQUALS(szBrace, "}"))
										break;

									char* szContext = NULL;
									char* szCondition = strtok_s(strtrlws(szBuffer), " ,=\n", &szContext);
									char* szResultingState = strtok_s(NULL, " ,=\n", &szContext);

									strsubst(szResultingState, '\0', ",");

									RegisterTransition(strStateName.c_str(), szCondition, szResultingState);
								}
							}
						}
					}
				}
			} // Transitions
		}
	}

	return true;
}
// Author: Stanley Taveras