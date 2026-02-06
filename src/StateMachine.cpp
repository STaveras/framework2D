// File: StateMachine.cpp

// TODO: Use Lua to load the transition table instead of your pseudo-Lua-like script?
// TODO: Add the ability to filter events by sender

#include "StateMachine.h"
#include "State.h"
#include "StrUtil.h"

#include <iostream>
#include <fstream>
#include <string.h>

using namespace FileSystem;

// Originally, I meant for this to subscribe to EVT_STATEMACHINE_EVENT(s) and react to them...
//void StateMachine::_onEvent(const StateMachineEvent& evt)
//{
//   if (_isBuffered) {
//      if (!containsCondition(evt.getCondition())) {
//         _events.push(evt);
//      }
//   }
//   else if (State* state = _nextState(evt)) { // This is where the NULL check has to appear (:
//      setState(state);      
//   }
//}

StateMachine::StateMachine(void) :
   _state(NULL),
   _isBuffered(false),
   _transitionFrequency(0.0f),
   _transitionTimer(0.0f) {

}

StateMachine::~StateMachine(void) {
   _transitionTable.clear();
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

void StateMachine::setState(State* state)
{
   if (state) {

      if (state == _state)
         return;

      if (_state) {
         _state->onExit(state);
      }
      state->onEnter(_state);
   }
   
   // We're allowing externally setting of "NULL"
   _state = state;
}

State* StateMachine::getState(const char* szName)
{
   Factory<State>::const_factory_iterator itr = this->begin();
   for (; itr != this->end(); itr++) {
      if (!strcmp(szName, (*itr)->getName()))
         return (*itr);
   }
   return NULL;
}

State* StateMachine::addState(const char* name)
{
   State* state = this->create();
   state->setName(name);
   return state;
}

void StateMachine::addTransition(const char* condition, const char* nextState)
{
   registerTransition(this->at((unsigned int)(this->size() - 1))->getName(), condition, nextState);
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

void StateMachine::start(void)
{
   if (!_state) {
      if (!this->empty())
         this->setState(this->at(0));
   }
}

void StateMachine::finish(void)
{
   while (!_events.empty())
      _events.pop();
}

void StateMachine::sendInput(const char* condition, void* sender)
{
   //this->_onEvent(StateMachineEvent(condition, sender));
	void* eventSender = sender ? sender : this;
	if (_isBuffered) {
		if (!containsCondition(condition)) {
			_events.push(StateMachineEvent(condition, eventSender));
		}
	}
	else if (State* state = _nextState(StateMachineEvent(condition, eventSender))) {
		setState(state);
	}
}

void StateMachine::update(float fTime)
{   
   if (_state && !_state->onExecute(fTime)) {
      this->sendInput(EVT_STATE_END, this); // An internal condition...
   }
   if (_isBuffered && !_events.empty()) {

      _transitionTimer += fTime;

      if (_transitionTimer >= _transitionFrequency) {

         if (State* nextState = _nextState(_events.front())) {
            setState(nextState);
         }

         _transitionTimer = 0.0f;
         _events.pop();
      }
   }
}

// I'd like to be able to look at the queue and see what's in it
bool StateMachine::containsCondition(const char* condition)
{
   // *should* be a copy by value operation (so it might be potentially slow)
   std::queue<StateMachineEvent> tempQueue = _events; 

   while (!tempQueue.empty()) {
      if (!strcmp(tempQueue.front().getCondition(), condition)) {
         return true;
      }
      tempQueue.pop();
   }

   return false;
}

void StateMachine::toJSON(std::ostream& fileStream)
{
   std::string json;

   // Begin JSON object
   json += "{\n";

   // Serialize isBuffered property
   json += "\t\"isBuffered\": " + std::string(_isBuffered ? "true" : "false") + ",\n";

   // Serialize transitionFrequency property
   json += "\t\"transitionFrequency\": " + std::to_string(_transitionFrequency) + ",\n";

   // Serialize transitions
   if (!_transitionTable.empty())
   {
      json += "\t\"transitions\": [\n";
      for (const auto& transition : _transitionTable) {
         json += "\t\t{\n";
         json += "\t\t\t\"state\": \"" + std::string(transition.first->getName()) + "\",\n";
         json += "\t\t\t\"event\": \"" + std::string(transition.second.first.getCondition()) + "\",\n";
         json += "\t\t\t\"resultState\": \"" + std::string(transition.second.second->getName()) + "\"\n";
         json += "\t\t},\n";
      }
      json.pop_back(); json.pop_back(); // Remove the last comma
		json += "\n\t],\n";
   }

   // Serialize events
   if (!_events.empty()) 
   {
      json += "\t\"events\": [\n";
      while (!_events.empty()) {
         json += "\t\t\"" + std::string(_events.front().getCondition()) + "\",\n";
         _events.pop();
      }

		json.pop_back(); 
		json.pop_back(); 
      json += "\n\t]\n";
   }
   else {
      json.pop_back();
      json.pop_back();
      json += "\n";
   }

   // End JSON object
   json += "}";

   // Save the JSON string to the file
   fileStream << json;
#if _DEBUG
   std::cout << "State machine information saved to file stream." << std::endl;
#endif
}

void StateMachine::toJSON(const std::string& filename)
{
   FileStream fileStream = File::Open(filename, false, true);

   if (!fileStream.is_open()) {
      std::cerr << "Failed to open file: " << filename << std::endl;
      return;
   }

   toJSON(fileStream);

   fileStream.close();
}

void StateMachine::fromJSON(std::istream& fileStream) 
{
   // Load the contents of the JSON file
   std::string jsonString((std::istreambuf_iterator<char>(fileStream)), std::istreambuf_iterator<char>());

   // Parse the JSON using simdjson
   simdjson::dom::parser parser;
   simdjson::padded_string paddedJson(jsonString);
   simdjson::simdjson_result<simdjson::dom::element> rootResult = parser.parse(paddedJson);
   if (rootResult.error()) {
      std::cerr << "Failed to parse StateMachine JSON: " << rootResult.error() << std::endl;
      return;
   }
   simdjson::dom::element root = rootResult.value();

   // Deserialize the StateMachine properties from the JSON
   _isBuffered = root["isBuffered"].get_bool();
   _transitionFrequency = (float)root["transitionFrequency"].get_double();

   // Deserialize transitions
   if (root["transitions"].is_array()) 
   {
      for (simdjson::dom::element transition : root["transitions"]) 
      {
         auto stateView = transition["state"].get_string().value_unsafe();
         auto eventView = transition["event"].get_string().value_unsafe();
         auto resultStateView = transition["resultState"].get_string().value_unsafe();
         
         std::string stateString(stateView);
         std::string eventString(eventView);
         std::string resultState(resultStateView);
         
         this->registerTransition(stateString.c_str(), eventString.c_str(), resultState.c_str());
      }
   }

   // Deserialize events
   if (root["events"].is_array()) {
      for (simdjson::dom::element event : root["events"]) {
         auto eventView = event.get_string().value_unsafe();
         std::string eventCondition(eventView);
         this->sendInput(eventCondition.c_str());
      }
   }
}
