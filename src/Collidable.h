// File: Collidable.h
#pragma once
#include "COL_OBJ_TYPE.h"
#include "Types.h"
class Collidable
{
	COL_OBJ_TYPE m_eType;

protected:
	vector2 m_Position;
	Collidable(COL_OBJ_TYPE type):m_eType(type){}

public:
	vector2 GetPosition(void) const { return m_Position; }
	void SetPosition(const vector2& position) { m_Position = position; }

	virtual bool Check(const Collidable* colObject) = 0;
};
// Author: Stanley Taveras