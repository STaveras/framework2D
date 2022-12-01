// File: ProgramStack.h
// Author: Stanley Taveras
// Created: 2/23/2010
// Modified: 2/23/2010

#if !defined(_PROGRAMSTACK_H_)
#define _PROGRAMSTACK_H_

#include "ProgramState.h"

#include <stack>

class ProgramStack : private std::stack<ProgramState*>
{
	typedef std::stack<ProgramState*> _ProgramStack;

public:
	ProgramStack(void) {}
	~ProgramStack(void) {}

	using _ProgramStack::empty;
	using _ProgramStack::size;
	using _ProgramStack::top;

	void push(ProgramState* state);
	void pop(void);
	void clear(void);
};

#endif