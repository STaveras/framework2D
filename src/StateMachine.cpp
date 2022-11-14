// File: StateMachine.cpp

// TODO: Use Lua to load the transition table instead of your pseudo-Lua-like script?
// TODO: Add the ability to filter events by sender

#include "StateMachine.h"
#include "State.h"
#include "StrUtil.h"

#include <fstream>
#include <string.h>

StateMachine::StateMachine(void) :
   _isBuffered(false),
   _transitionFrequency(0.0f),
   _transitionTimer(0.0f) ,
   _state(NULL) {

}

StateMachine::~StateMachine(void) {
   // Innecessary but for completeness' sake
   _transitionTable.clear();
}

// ...I want to decouple this
void StateMachine::OnEvent(const StateMachineEvent& evt)
{
   if (_isBuffered)
      _events.push(evt);
   else
      _transitionTo(_nextState(evt));
}

State* StateMachine::_nextState(const StateMachineEvent& evt)
{
   std::pair<std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator,
             std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator> range = _transitionTable.equal_range(_state);

   std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator itr = range.first;

   for (; itr != range.second; itr++) {
      if (itr->second.first == evt)
         return itr->second.second;
   }

   return NULL;
}

void StateMachine::_transitionTo(State* nextState)
{
   if (nextState) {

      if (_state) {
         _state->onExit(nextState);
      }

      nextState->onEnter(_state);

      _state = nextState;
   }
}

State* StateMachine::getState(const char* szName)
{
   Factory<State>::const_factory_iterator itr = this->Begin();

   for (; itr != this->End(); itr++) {
      if (!strcmp(szName, (*itr)->getName()))
         return (*itr);
   }

   return NULL;
}

State* StateMachine::addState(const char* name)
{
   State* state = this->Create();
   state->setName(name);

   return state;
}

void StateMachine::addTransition(const char* condition, const char* nextState)
{
   registerTransition(this->At(this->Size() - 1)->getName(), condition, nextState);
}

void StateMachine::registerTransition(State* state, const StateMachineEvent& evt, State* resultingState)
{
   _transitionTable.insert(std::make_pair(state, std::make_pair(evt, resultingState)));
}

void StateMachine::registerTransition(const char* stateName, const char* condition, const char* resultingState, void* sender)
{
   State* state = getState(stateName);
   State* nextState = getState(resultingState);

   if (state && nextState) {
      _transitionTable.insert(std::make_pair(state, std::make_pair(StateMachineEvent(condition, sender), nextState)));
   }
}

void StateMachine::initialize(void)
{
   if (!this->Empty())
       this->_transitionTo(this->At(0));
}

void StateMachine::reset(void)
{
   this->initialize();
   
   while (!_events.empty())
      _events.pop();
}

void StateMachine::sendInput(const char* condition, void* sender)
{
   this->OnEvent(StateMachineEvent(condition, sender));
}

void StateMachine::update(float fTime)
{
   if (_state && !_state->onExecute(fTime)) {
      this->sendInput(EVT_STATE_END, this); // An internal condition...
   }
   else if (_isBuffered && !_events.empty())
   {
      _transitionTimer += fTime;

      if (_transitionTimer >= _transitionFrequency)
      {
         _transitionTo(_nextState(_events.front()));

         _transitionTimer = 0.0f;

         _events.pop();
      }
   }
}

// This was before I knew JSON is a thing
bool StateMachine::loadTransitionTableFromFile(const char* szFilename)
{
   std::ifstream ifl(szFilename);

   if (!ifl.good())
   {
      ifl.close();
      ifl.clear();

      return false;
   }
   else
   {
      char szBuffer[256];

      while (ifl.good())
      {
         ifl.getline(szBuffer, 256);

         if (!strcmp(szBuffer, "States"))
         {
            ifl.getline(szBuffer, 256);

            if (STR_EQUALS(szBuffer, "{"))
            {
               while (true)
               {
                  ifl.getline(szBuffer, 256);

                  if (STR_EQUALS(szBuffer, "}"))
                     break;

                  char* szStateName = strtrlws(szBuffer);
                  strsubst(szStateName, '\0', ",");

                  if (!this->getState(szStateName) && !STR_EQUALS(szStateName, ""))
                     this->Create(State(szStateName));

                  // We used to be able to explicitly set the initial state... 
                  //if (!m_pStartState && GetState(szStateName))
                  //   m_pStartState = GetState(szStateName);
               }
            }
         } // States
         else if (!strcmp(szBuffer, "Transitions"))
         {
            ifl.getline(szBuffer, 256);
            char* szBrace = strtrlws(szBuffer);

            if (STR_EQUALS(szBrace, "{"))
            {
               while (true)
               {
                  ifl.getline(szBuffer, 256);
                  szBrace = strtrlws(szBuffer);

                  if (STR_EQUALS(szBrace, "}"))
                     break;

                  std::string strStateName = strtrlws(szBuffer);

                  State* pState = getState(strStateName.c_str());

                  if (pState)
                  {
                     ifl.getline(szBuffer, 256);
                     szBrace = strtrlws(szBuffer);

                     if (STR_EQUALS(szBrace, "{"))
                     {
                        while (true)
                        {
                           ifl.getline(szBuffer, 256);
                           szBrace = strtrlws(szBuffer);

                           if (STR_EQUALS(szBrace, "}"))
                              break;

                           char* szContext = NULL;
                           char* szCondition = strtok_s(strtrlws(szBuffer), " ,=\n", &szContext);
                           char* szResultingState = strtok_s(NULL, " ,=\n", &szContext);

                           strsubst(szResultingState, '\0', ",");

                           registerTransition(strStateName.c_str(), szCondition, szResultingState);
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