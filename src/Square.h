// File: Square.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 7/21/2022

#if !defined(_SQUARE_H)
#define _SQUARE_H

#include "Collidable.h"

class Circle;
class Plane;

// Probably should be renamed 'rectangle' or 'box'
// Shouldn't be "Collidable" but instead just a general "Geometry" parent class
class Square : public Collidable
{
	float _fWidth = 0;
	float _fHeight = 0;
  
public:
   Square(vector2 position, vector2 max) : Collidable(COL_OBJ_SQUARE), _fWidth(max.x - position.x), _fHeight(max.y - position.y) { _position = position; }
	Square(vector2 position, float width, float height) : Collidable(COL_OBJ_SQUARE) { _position = position; _fWidth = width; _fHeight = height; }

	//vector2 GetMin(void) const { return _position; }
	vector2 getMax(void) const { return vector2(this->getPosition().x + _fWidth, this->getPosition().y + _fHeight); }
	//float getWidth(void) const { return (m_Max.x - _position.x); }
	//float getHeight(void) const { return (m_Max.y - _position.y); }
	float getWidth(void) const { return _fWidth; }
	float getHeight(void) const { return _fHeight; }

	void setWidth(float fWidth) { _fWidth = fWidth; }
	void setHeight(float fHeight) { _fHeight = fHeight; }
	//void SetMin(const vector2& min) { _position = min; }
	//void SetMax(const vector2& max) { m_Max = max; }
	//void setWidth(float fWidth) { m_Max.x = _position.x + fWidth; }
	//void setHeight(float fHeight) { m_Max.y = _position.y + fHeight; }

  bool collidesWith(const Collidable * collidable);
  bool collidesWith(vector2 point);
  // TODO: Do this in better way or move to an HPP?

  // TODO: Add 
};

#endif  //_SQUARE_H
