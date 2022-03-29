// File: State.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 1/30/2018

#if !defined(_STATE_H)
#define _STATE_H

#include <string>

class State
{
   char* m_szName;

public:
   State(void) :m_szName(NULL) {}
   State(const char* szName) :m_szName( _strdup(szName) ) {}
   State(const State& rhs) : m_szName(_strdup(rhs.m_szName)) {}
   virtual ~State(void) { if (m_szName) free(m_szName); }

   const char* GetName(void) const { return m_szName; }
   void SetName(const char* szName) { if (m_szName) free(m_szName); m_szName = _strdup(szName); }

   //State& SetName(const char* szName) { 

   //   if (m_szName) 
   //      free(m_szName); 

   //   m_szName = _strdup(szName); 

   //   return (*this);
   //}

   virtual void onEnter(State* prevState) {} // EVT_PREV_TRANSITION FROM
   virtual bool onExecute(float fTime) { return false; }
   virtual void onExit(State* nextState) {}  // EVT_WILL_TRANSITION TO
};

#endif  //_STATE_H
