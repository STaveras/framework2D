// File: Strategy.h
// Author: Stanley Taveras
// Created: 2/25/2010
// Modified: 2/25/2010

#if !defined(_STRATEGY_H_)
#define _STRATEGY_H_

class Strategy
{
public:
	virtual void Execute(void* pObj) const = 0;
	virtual void operator()(void* pObj) const = 0;
};

#endif