// File: Circle.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_CIRCLE_H)
#define _CIRCLE_H

#include "Collidable.h"

class Plane;
class Square;

class Circle : public Collidable
{
	float m_fRadius;

public:
	bool WithCircle(const Circle* circle);
	bool WithPlane(const Plane* plane);
	bool WithSquare(const Square* square);
};

#endif  //_CIRCLE_H
