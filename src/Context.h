// File: Context.h
// Author: Stanley Taveras
// Created: 2/25/2010
// Modified: 2/25/2010

#if !defined(_CONTEXT_H_)
#define _CONTEXT_H_

#include "Strategy.h"

class Context
{
	friend Strategy;

	Strategy* m_pStrategy;

public:
	explicit Context(Strategy* pStrat): m_pStrategy(pStrat) {}

	Strategy* GetStrategy(void) const { return m_pStrategy; }
	void SetStrategy(Strategy* pStrategy) { m_pStrategy = pStrategy; }

	void ExecuteStrategy(void) { m_pStrategy->Execute(this); }
};

#endif