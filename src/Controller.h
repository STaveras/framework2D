// File: Controller.h
#pragma once

#include "IInput.h"
#include "Action.h"
#include <list>

class Controller
{
   bool _connected;
   int m_nPadNumber;
   IInput* _input;
   std::list<Action> _actions;

public:
   Controller(void) : _connected(false), m_nPadNumber(-1), _input(NULL) {}
   ~Controller(void) {}

   bool isConnected(void) const { return _connected; }
   int getPadNumber(void) const { return m_nPadNumber; }

   IInput* getInputInterface(void) { return _input; }

   void setIsConnected(bool bConnected) { _connected = bConnected; }
   void setPadNumber(int nPadNumber) { m_nPadNumber = nPadNumber; }
   void setInputInterface(IInput* pInput) { _input = pInput; }

   void addAction(Action action) { _actions.push_back(action); }
   void removeAction(Action action);

   bool buttonPressed(std::string actionName);
   bool buttonReleased(std::string actionName);

   Action* getAction(std::string actionName);
   std::list<Action>& getActions(void) { return _actions; }
};

typedef Controller controller;
// Author: Stanley Taveras