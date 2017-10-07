// File: Square.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_SQUARE_H)
#define _SQUARE_H

#include "Collidable.h"

class Circle;
class Plane;

class Square : public Collidable
{
	vector2 m_Max;

public:
	vector2 GetMin(void) const { return m_Position; }
	vector2 GetMax(void) const { return m_Max; }
	float GetWidth(void) const { return (m_Max.x - m_Position.x); }
	float GetHeight(void) const { return (m_Max.y - m_Position.y); }

	void SetMin(const vector2& min) { m_Position = min; }
	void SetMax(const vector2& max) { m_Max = max; }
	void SetWidth(float fWidth) { m_Max.x = m_Position.x + fWidth; }
	void SetHeight(float fHeight) { m_Max.y = m_Position.y + fHeight; }

	bool WithCircle(const Circle* circle);
	bool WithPlane(const Plane* plane);
	bool WithSquare(const Square* square);
};

#endif  //_SQUARE_H
