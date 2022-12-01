// File: ObjectOperator.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_OBJECTOPERATOR_H)
#define _OBJECTOPERATOR_H

class GameObject;

// Used to apply rules affecting an object
class ObjectOperator
{
	bool _enabled = false;

public:

	bool isEnabled(void) const { return _enabled; }
	void setEnabled(bool enable) { _enabled = true; }

	virtual ~ObjectOperator(void) {}

	virtual bool operator()(GameObject* object) = 0; // TODO: Make it return a status code?
};

#endif  //_OBJECTOPERATION_H
