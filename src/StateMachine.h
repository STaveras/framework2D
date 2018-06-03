// File: StateMachine.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 6/29/2010

#if !defined(_STATEMACHINE_H)
#define _STATEMACHINE_H

#include "Factory.h"
#include "State.h"
#include "StateMachineEvent.h"
#include <list>
#include <map>
#include <queue>

class StateMachine
{
	bool m_bBuffered;
	float m_fTransitionFrequency;
	State* m_pStartState;
	State* m_pState;

	void OnEvent(const StateMachineEvent& evt);

protected:
	Factory<State> _states;
	std::queue<StateMachineEvent> m_qEvents;
	std::multimap<State*, std::pair<StateMachineEvent, State*>> m_mTransitionTable;

	State* _GetNextState(const StateMachineEvent& evt);
	void _Transition(State* pNextState);

public:
	StateMachine(void);
	~StateMachine(void);

	void SetIsBuffered(bool bBuffered) { m_bBuffered = bBuffered; }
	void SetTransitionFrequency(float fFreq) { m_fTransitionFrequency = fFreq; }
	void SetStartState(State* pState) { m_pStartState = pState; }
	void SetState(State* pState) { if(m_pState) m_pState->onExit(pState); pState->onEnter(m_pState); m_pState = pState; } // Should I even allow this?

	bool IsBuffered(void) const { return m_bBuffered; }
	float GetTransitionFrequency(void) const { return m_fTransitionFrequency; }
	State* GetCurrentState(void) const { return m_pState; }
	State* GetStartState(void) const { return m_pStartState; }
	State* GetState(const char* szName);
	Factory<State>* GetStateFactory(void) { return &_states; }

	State* AddState(const char* name);
	void AddTransition(const char* condition, const char* nextState);
	void RegisterTransition(State* pState, const StateMachineEvent& evt, State* pStateResult);
	void RegisterTransition(const char* szStateName, const char* szCondition, const char* szResultState);

	void Initialize(void);
	void Reset(void);

	void SendInput(const char* szCondition, void* pSender = NULL);

	virtual void update(float fTime);

	// We should create a templated version of this so clients can initialize states 
	// using a derived State class
	bool LoadTransitionTableFromFile(const char* szFilename);
};

#endif  //_STATEMACHINE_H
