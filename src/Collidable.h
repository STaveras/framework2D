// File: Collidable.h
#pragma once
#include "COL_OBJ_TYPE.h"
#include "Positionable.h"
#include "Types.h"
class Collidable: public Positionable
{
	COL_OBJ_TYPE m_eType;

protected:
   Collidable(COL_OBJ_TYPE type):m_eType(type){}

public:
	virtual bool Check(const Collidable* colObject) = 0;
};
// Author: Stanley Taveras