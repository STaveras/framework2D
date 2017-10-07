// File: Plane.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_PLANE_H)
#define _PLANE_H

#include "Collidable.h"

class Circle;
class Square;

class Plane : public Collidable
{
	vector2 m_Normal;

public:
	vector2 GetNormal(void) const { return m_Normal; }
	void SetNormal(const vector2& normal) { m_Normal = normal; }

	bool WithCircle(const Circle* circle);
	bool WithPlane(const Plane* plane);
	bool WithSquare(const Square* square);
};

#endif  //_PLANE_H
