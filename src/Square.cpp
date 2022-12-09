// File: Square.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "Square.h"
#include "Plane.h"

bool WithCircle(const Square* square, const Circle* circle)
{
	return false;
}

bool WithPlane(const Square* square, const Plane* plane)
{
	//vector2 position = ((_position + m_Max) / 2);

	//vector2 toSquare = position - plane->getPosition();

	//vector2 width = (m_Max - _position);

	//// TODO : Remove DirectX-specific code
	//float fRadius = D3DXVec2Length(&width) / 2;
	//float fDotResult = D3DXVec2Dot(&toSquare, &plane->getNormal());

	// if(fDotResult < fRadius)
	//	   return true;

	return false;
}

bool WithSquare(const Square* squareA, const Square* squareB)
{
	if (squareA->getPosition().x > squareB->getMax().x ||
		squareA->getPosition().y > squareB->getMax().y ||
		squareA->getMax().x < squareB->getPosition().x ||
		squareA->getMax().y < squareB->getPosition().y)
		return false;

	return true;
}

bool Square::collidesWith(vector2 point)
{
	if (point.x >= this->getPosition().x && point.y >= this->getPosition().y &&
		point.x <= this->getMax().x && point.y <= this->getMax().y)
		return true;

	return false;
}

// This function is likely to look the same for all of these objects...
bool Square::collidesWith(const Collidable* collidable)
{
   switch (collidable->getType()) {
   case COL_OBJ_SQUARE:
      return WithSquare(this, (Square*)collidable);
   default:
		return this->collidesWith(collidable->getPosition());
   }
}