// File: ProgramStack.cpp
#include "ProgramStack.h"

void ProgramStack::push(IProgramState* state)
{
	std::stack<IProgramState*>::push(state);
	this->top()->onEnter();
}

void ProgramStack::pop(void)
{
	if (!this->empty())
	{
		this->top()->onExit();

		std::stack<IProgramState*>::pop();
	}
}

void ProgramStack::update(float fTime)
{
	if(!this->empty())
		this->top()->onExecute(fTime);
}

void ProgramStack::clear(void)
{
	while(!this->empty())
	{
		this->top()->onExit();
		this->pop();
	}
}
// Author: Stanley Taveras