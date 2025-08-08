// File: CullObjectOperator.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_CULLOBJECTOPERATOR_H)
#define _CULLOBJECTOPERATOR_H

#include "Camera.h"
#include "ObjectOperator.h"

class GameObject;

// I realized this is only for not updating an object if it is not on screen, its renderable can still be on screen

class CullObjectOperator : public ObjectOperator
{
	Camera* _camera = NULL;

public:
	bool operator()(GameObject* object);

	Camera* getCamera(void) const { return _camera; }
	void setCamera(Camera* camera) { _camera = camera; }
};

#endif  //_CULLOBJECTOPERATOR_H
