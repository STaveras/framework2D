// File: StateMachine.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 11/12/2022

#if !defined(_STATEMACHINE_H)
#define _STATEMACHINE_H

#include "Factory.h"
#include "Cyclable.h"
#include "State.h"
#include "StateMachineEvent.h"

#include <list>
#include <map>
#include <queue>

#define EVT_STATE_END "EVT_STATE_END"

class StateMachine : public Factory<State>, Cyclable
{
	State* _state;

	bool  _isBuffered;
	float _transitionFrequency;
	float _transitionTimer;

	//void _onEvent(const StateMachineEvent& evt);

protected:
	std::multimap<State*, std::pair<StateMachineEvent, State*>> _transitionTable;
	std::queue<StateMachineEvent> _events;

	State* _nextState(const StateMachineEvent& evt);

public:
	StateMachine(void);
	~StateMachine(void);

	bool isBuffered(void) const { return _isBuffered; }
	void setBuffered(bool bBuffered) { _isBuffered = bBuffered; }

	float getTransitionFrequency(void) const { return _transitionFrequency; }
	void setTransitionFrequency(float fFreq) { _transitionFrequency = fFreq; }

	State* addState(const char* name);

	State* getState(void) const { return _state; }
	State* getState(const char* szName);

	void setState(State* state);
	void setState(const char* name) {
		this->setState(getState(name));
	}

	void addTransition(const char* condition, const char* nextState);
	void registerTransition(State* pState, const StateMachineEvent& evt, State* pStateResult);
	void registerTransition(const char* szStateName, const char* szCondition, const char* szResultState, void* sender = NULL);

	virtual void start(void);
	virtual void update(float fTime);
	virtual void finish(void);

	void sendInput(const char* condition, void* sender = NULL);
	bool containsCondition(const char* condition);
	void clearEvents(void) { StateMachine::finish(); }

	// We should create a templated version of this so clients can initialize states 
	// using a derived State class
	bool loadTransitionTableFromFile(const char* szFilename);

	void toJSON(const std::string& filename);
	void toJSON(std::ostream& fileStream);

	void fromJSON(std::istream& fileStream);
};

#endif  //_STATEMACHINE_H
