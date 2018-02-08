// File: CullObjectOperator.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_CULLOBJECTOPERATOR_H)
#define _CULLOBJECTOPERATOR_H

#include "ObjectOperator.h"

class Camera;
class GameObject;

// I don't even... Why does this exist as an operator? Put it in the renderer? I don't want collision code in there but...

class CullObjectOperator : public ObjectOperator
{
	Camera* m_pCamera;

public:
	bool operator()(GameObject* object);
};

#endif  //_CULLOBJECTOPERATOR_H
