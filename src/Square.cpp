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
	//float fDotResult = D3DXVec2Dot(&toSquare, &plane->GetNormal());

	// if(fDotResult < fRadius)
	//	   return true;

	return false;
}

bool WithSquare(const Square* squareA, const Square* squareB)
{
#if _WIN32 && !defined(GLM)
	if (squareA->getPosition() > squareB->getMax() || squareA->getMax() < squareB->getPosition())
#else
	if (glm::all(glm::greaterThanEqual(squareA->getPosition(), squareB->getMax())) && 
		glm::all(glm::lessThanEqual(squareA->getMax(), squareB->getPosition())))
#endif
		return false;
	
	return true;
}

bool Square::Check(vector2 point)
{
#if _WIN32 && !defined(GLM)
	// For whatever reason, just using > or < wouldn't work?
	//if (point >= this->getPosition() && point <= this->GetMax())

	if (point.x >= this->getPosition().x && point.y >= this->getPosition().y && 
		point.x <= this->getMax().x && point.y <= this->getMax().y)
#else
	// I wish GLM just overrode the comparison operators
	if (glm::all(glm::greaterThan(point, _position)) && glm::all(glm::lessThan(point, this->getMax())))
#endif
      return true;

   return false;
}

// This function is likely to look the same for all of these objects...
bool Square::Check(const Collidable* collidable)
{
   switch (collidable->getType()) {
   case COL_OBJ_SQUARE:
      return WithSquare(this, (Square*)collidable);
   default:
		return this->Check(collidable->getPosition());
   }
}