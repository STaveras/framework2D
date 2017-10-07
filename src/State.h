// File: State.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 7/4/2010

#if !defined(_STATE_H)
#define _STATE_H

#include <string>

class State
{
	char* m_szName;

public:
	State(void):m_szName(NULL){}
	State(const char* szName):m_szName(_strdup(szName)){}
	State(const State& rhs): m_szName(_strdup(rhs.m_szName)) {}
	virtual ~State(void) { if(m_szName) free(m_szName); }

	const char* GetName(void) const { return m_szName; }
	void SetName(const char* szName) { if(m_szName) free(m_szName); m_szName = _strdup(szName); }

	virtual void OnEnter(State* prevState){}
	virtual bool OnExecute(float fTime){ return false; }
	virtual void OnExit(){}
};

#endif  //_STATE_H
