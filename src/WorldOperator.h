// File: WorldOperator.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_WORLDOPERATOR_H)
#define _WORLDOPERATOR_H

#include "ObjectOperator.h"

class GameObject;
class Stage;

class WorldOperator : public ObjectOperator
{
	float m_fGravity;
	Stage* m_pStage;

public:
	bool operator()(GameObject* object);
};

#endif  //_WORLDOPERATOR_H
