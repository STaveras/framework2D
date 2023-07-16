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
	float _width = 0;
	float _height = 0;
  
public:
	Square(void) : Collidable(COL_OBJ_SQUARE) {}
	Square(const Square& s) : Collidable(COL_OBJ_SQUARE), _width(s._width), _height(s._height) { _position = s._position; }
   Square(vector2 min, vector2 max) : Collidable(COL_OBJ_SQUARE), _width(max.x - min.x), _height(max.y - min.y) { _position = min; }
	Square(vector2 position, float width, float height) : Collidable(COL_OBJ_SQUARE) { _position = position; _width = width; _height = height; }

	vector2 getMin(void) const { return _position; } // just because???
	vector2 getMax(void) const { return vector2(this->getPosition().x + _width, this->getPosition().y + _height); }

	float getWidth(void) const { return _width; }
	float getHeight(void) const { return _height; }

	void setWidth(float width) { _width = width; }
	void setHeight(float height) { _height = height; }
	void setMin(const vector2& min) { _position = min; }
	void setMax(const vector2& max) {
		 _width = max.x - _position.x;
		_height = max.y - _position.y;
	}

  bool collidesWith(const Collidable * collidable);
  bool collidesWith(vector2 point);
};

#endif  //_SQUARE_H
