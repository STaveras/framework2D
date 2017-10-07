// File: IProgramState.h
// Author: Stanley Taveras
// Created: 2/23/2010
// Modified: 2/23/2010

#if !defined(_IPROGRAMSTATE_H_)
#define _IPROGRAMSTATE_H_

class IProgramState
{
public:
	IProgramState(void){}
	virtual ~IProgramState(void) = 0{}

	virtual void OnEnter(void) = 0{}
	virtual void OnExecute(float fTime) = 0{}
	virtual void OnExit(void) = 0{}
};

#endif