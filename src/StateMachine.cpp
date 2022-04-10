// File: StateMachine.cpp

// TODO: Use Lua to load the transition table instead of your pseudo-Lua-like script?
// TODO: Add the ability to filter events by sender

#include "StateMachine.h"
#include "State.h"
#include "StrUtil.h"

#include <fstream>
#include <string.h>

StateMachine::StateMachine(void) :
   m_bBuffered(false),
   m_fTransitionFrequency(1.0f),
   m_pStartState(NULL),
   m_pState(NULL)
{}

StateMachine::~StateMachine(void) {
   // Innecessary but for completeness' sake
   m_mTransitionTable.clear();
   _states.Clear();
}

State* StateMachine::_GetNextState(const StateMachineEvent& evt)
{
   std::pair<std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator,
             std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator> range = m_mTransitionTable.equal_range(m_pState);

   std::multimap<State*, std::pair<StateMachineEvent, State*>>::iterator itr = range.first;

   for (; itr != range.second; itr++) {
      if (itr->second.first == evt)
         return itr->second.second;
   }

   return NULL;
}

void StateMachine::_Transition(State* pNextState)
{
   if (pNextState) {

      if (m_pState)
         m_pState->onExit(pNextState);

      pNextState->onEnter(m_pState);

      m_pState = pNextState;
   }
}

State* StateMachine::GetState(const char* szName)
{
   Factory<State>::const_factory_iterator itr = _states.Begin();

   for (; itr != _states.End(); itr++) {
      if (!strcmp(szName, (*itr)->GetName()))
         return (*itr);
   }

   return NULL;
}

State* StateMachine::AddState(const char* name)
{
   State* state = _states.Create();
   state->SetName(name);

   return state;
}

void StateMachine::addTransition(const char* condition, const char* nextState)
{
   registerTransition(_states.At(_states.Size() - 1)->GetName(), condition, nextState);
}

void StateMachine::registerTransition(State* pState, const StateMachineEvent& evt, State* pStateResult)
{
   m_mTransitionTable.insert(std::make_pair(pState, std::make_pair(evt, pStateResult)));
}

void StateMachine::registerTransition(const char* szStateName, const char* szCondition, const char* szResultState)
{
   State* pState = GetState(szStateName);
   State* pStateResult = GetState(szResultState);

   if (pState && pStateResult)
   {
      StateMachineEvent evt(szCondition, NULL);
      m_mTransitionTable.insert(std::make_pair(pState, std::make_pair(evt, pStateResult)));
   }
}

void StateMachine::initialize(void)
{
   if (!m_pStartState && _states.Size() > 0)
      m_pStartState = _states.At(0);

   if (m_pStartState) {
      m_pState = m_pStartState;
      m_pState->onEnter(NULL); // Should this even really be done...?
   }
}

void StateMachine::reset(void)
{
   if (m_pState)
      m_pState->onExit(m_pStartState);

   if (m_pStartState) {
      m_pState = m_pStartState;
      //m_pState->onEnter(NULL);
   }
   
   while (!m_qEvents.empty())
      m_qEvents.pop();
}

void StateMachine::OnEvent(const StateMachineEvent& evt)
{
   if (m_bBuffered)
      m_qEvents.push(evt);
   else
      _Transition(_GetNextState(evt));
}

void StateMachine::SendInput(const char* szCondition, void* pSender)
{
   this->OnEvent(StateMachineEvent(szCondition, pSender));
}

void StateMachine::update(float fTime)
{
   if (m_pState && !m_pState->onExecute(fTime) && m_pStartState)
      SetState(m_pStartState);

   if (m_bBuffered && !m_qEvents.empty())
   {
      static float fTransitionTimer = 0.0f;
      fTransitionTimer += fTime;

      if (fTransitionTimer >= m_fTransitionFrequency)
      {
         _Transition(_GetNextState(m_qEvents.front()));
         m_qEvents.pop();

         fTransitionTimer = 0.0f;
      }
   }
}

// This is before I knew JSON was a thing
bool StateMachine::LoadTransitionTableFromFile(const char* szFilename)
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

                  if (!GetState(szStateName) && !STR_EQUALS(szStateName, ""))
                     _states.Create(State(szStateName));

                  if (!m_pStartState && GetState(szStateName))
                     m_pStartState = GetState(szStateName);
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

                  State* pState = GetState(strStateName.c_str());

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