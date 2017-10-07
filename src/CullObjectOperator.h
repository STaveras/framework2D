// File: CullObjectOperator.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_CULLOBJECTOPERATOR_H)
#define _CULLOBJECTOPERATOR_H

#include "ObjectOperator.h"

class Camera;
class GameObject;

class CullObjectOperator : public ObjectOperator
{
	Camera* m_pCamera;

public:
	bool operator()(GameObject* object);
};

#endif  //_CULLOBJECTOPERATOR_H
