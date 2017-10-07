// File: ProgramStack.cpp
#include "ProgramStack.h"

void ProgramStack::push(IProgramState* state)
{
	std::stack<IProgramState*>::push(state);
	this->top()->OnEnter();
}

void ProgramStack::pop(void)
{
	if (!this->empty())
	{
		this->top()->OnExit();
		std::stack<IProgramState*>::pop();
	}
}

void ProgramStack::update(float fTime)
{
	if(!this->empty())
		this->top()->OnExecute(fTime);
}

void ProgramStack::clear(void)
{
	while(!this->empty())
	{
		this->top()->OnExit();
		this->pop();
	}
}
// Author: Stanley Taveras