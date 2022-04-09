// File: IProgramState.h
// Author: Stanley Taveras
// Created: 2/23/2010
// Modified: 2/23/2010

#if !defined(_IPROGRAMSTATE_H_)
#define _IPROGRAMSTATE_H_

// TODO: Maybe inherent from 'State' ???

class IProgramState
{
public:
	IProgramState(void){}
	virtual ~IProgramState(void) = default;

	virtual void onEnter(void) = 0;
	virtual void onExecute(float fTime) = 0;
	virtual void onExit(void) = 0;
};

#endif