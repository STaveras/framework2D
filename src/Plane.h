// File: Plane.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_PLANE_H)
#define _PLANE_H

#include "Types.h"
#include "Collidable.h"

class Circle;
class Square;

class Plane : public Collidable
{
	vector2 m_Normal;

	// I want to change this to be v3 for 2D, and v4 for 3D

public:
	Plane(void):m_Normal(0,0){ }

	vector2 GetNormal(void) const { return m_Normal; }
	void SetNormal(const vector2& normal) { m_Normal = normal; }

	bool collidesWith(const Collidable* colObject);
};

#endif  //_PLANE_H
