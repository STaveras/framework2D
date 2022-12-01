// File: ProgramStack.cpp
#include "ProgramStack.h"

void ProgramStack::push(ProgramState* state)
{
	State* prev = (!this->empty()) ? this->top() : NULL;
	std::stack<ProgramState*>::push(state);
	this->top()->onEnter(prev);
}

void ProgramStack::pop(void)
{
	if (!this->empty()) {
		State* current = this->top();
		std::stack<ProgramState*>::pop();
		current->onExit((!this->empty()) ? this->top() : NULL);
	}
}

void ProgramStack::clear(void)
{
	while(!this->empty()) {
		 this->pop();
	}
}
// Author: Stanley Taveras