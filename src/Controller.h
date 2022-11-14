// File: Controller.h
#pragma once

#include "IInput.h"
#include "Action.h"
#include <list>

class Controller
{
   bool m_bConnected;
   int m_nPadNumber;
   IInput* m_pInput;
   std::list<Action> _actions;

public:
   Controller(void) :m_bConnected(false), m_nPadNumber(-1), m_pInput(NULL) {}
   ~Controller(void) {}

   bool isConnected(void) const { return m_bConnected; }
   int getPadNumber(void) const { return m_nPadNumber; }
   IInput* getInputInterface(void) { return m_pInput; }
   std::list<Action>& getActions(void) { return _actions; }
   Action* getActions(ActionName eKey);

   void setIsConnected(bool bConnected) { m_bConnected = bConnected; }
   void setPadNumber(int nPadNumber) { m_nPadNumber = nPadNumber; }
   void setInputInterface(IInput* pInput) { m_pInput = pInput; }

   void addAction(Action action) { _actions.push_back(action); }
   void removeAction(Action action);

   bool buttonPressed(ActionName actionName);
   bool buttonReleased(ActionName actionName);
};

typedef Controller controller;
// Author: Stanley Taveras