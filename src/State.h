// File: State.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 1/30/2018

#if !defined(_STATE_H)
#define _STATE_H

#include "Types.h"

class State
{
   char* m_szName;

public:
   State(void) :m_szName(NULL) {}
   State(const char* szName) : m_szName( _strdup(szName) ) {}
   State(const State& rhs) : m_szName(_strdup(rhs.m_szName)) {}

   virtual ~State(void) { if (m_szName) free(m_szName); }

   //State& operator=(const State& state) {
   //   if (m_szName) {
   //      free(m_szName);
   //   }
   //   m_szName = _strdup(state.m_szName);
   //}

   const char* getName(void) const { return m_szName; }
   void setName(const char* szName) { if (m_szName) free(m_szName); m_szName = _strdup(szName); }

   virtual void onEnter(State* prevState) {
	   //if (prevState) {
		  // prevState->onExit(this);
	   //}
   }
   virtual bool onExecute(float fTime) { return onExecute(); }
   virtual bool onExecute(void) { return false; }

   virtual void onExit(State* nextState) {
	   //if (nextState) {
		  // nextState->onEnter(this);
	   //}
   }  // EVT_WILL_TRANSITION TO

   virtual bool operator==(const State& state) const {
      return !strcmp(this->getName(), state.getName());
   }
};

#endif  //_STATE_H
