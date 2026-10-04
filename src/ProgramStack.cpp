// File: ProgramStack.cpp
#include "ProgramStack.h"

void ProgramStack::push(IProgramState* state)
{
	State* prev = (!this->empty()) ? this->top() : NULL;
	std::stack<IProgramState*>::push(state);
	this->top()->onEnter(prev);
}

void ProgramStack::pop(void)
{
	if (!this->empty()) {
		State* current = this->top();
		std::stack<IProgramState*>::pop();
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