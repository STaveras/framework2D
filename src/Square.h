// File: Square.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_SQUARE_H)
#define _SQUARE_H

#include "Collidable.h"

class Circle;
class Plane;

// Probably should be renamed 'rectangle' or 'box'
class Square : public Collidable
{
	vector2 m_Max;
  
public:
   Square(vector2 position, vector2 max) : Collidable(COL_OBJ_SQUARE), m_Max(max) { _position = position; }
   Square(vector2 position, float width, float height) : Collidable(COL_OBJ_SQUARE) { _position = position; m_Max = vector2(_position.x + width, _position.y + height); }

	vector2 GetMin(void) const { return _position; }
	vector2 GetMax(void) const { return m_Max; }
	float GetWidth(void) const { return (m_Max.x - _position.x); }
	float GetHeight(void) const { return (m_Max.y - _position.y); }

	void SetMin(const vector2& min) { _position = min; }
	void SetMax(const vector2& max) { m_Max = max; }
	void SetWidth(float fWidth) { m_Max.x = _position.x + fWidth; }
	void SetHeight(float fHeight) { m_Max.y = _position.y + fHeight; }

  bool Check(const Collidable * collidable);
  bool Check(vector2 point);

  // Not helpful since we have to have knowledge of the other collision object types; no generalized way of testing collision with any shape...
  bool WithCircle(const Circle* circle);
  bool WithPlane(const Plane* plane);
  bool WithSquare(const Square* square);
  // TODO: Do this in better way or move to an HPP?
};

#endif  //_SQUARE_H
