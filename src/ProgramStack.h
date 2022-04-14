// File: ProgramStack.h
// Author: Stanley Taveras
// Created: 2/23/2010
// Modified: 2/23/2010

#if !defined(_PROGRAMSTACK_H_)
#define _PROGRAMSTACK_H_

#include "IProgramState.h"
#include <stack>

class ProgramStack : private std::stack<IProgramState*>
{
	typedef std::stack<IProgramState*> _ProgramStack;

public:
	ProgramStack(void) {}
	~ProgramStack(void) { clear(); }

	using _ProgramStack::empty;
	using _ProgramStack::size;
	using _ProgramStack::top;

	void push(IProgramState* state);
	void pop(void);
	void update(float time);
	void clear(void);
};

#endif