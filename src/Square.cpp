// File: Square.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "Square.h"
#include "Plane.h"

bool Square::WithCircle(const Circle* circle)
{
	return false;
}

bool Square::WithPlane(const Plane* plane)
{
	//vector2 position = ((_position + m_Max) / 2);

	//vector2 toSquare = position - plane->getPosition();

	//vector2 width = (m_Max - _position);

	//// TODO : Remove DirectX-specific code
	//float fRadius = D3DXVec2Length(&width) / 2;
	//float fDotResult = D3DXVec2Dot(&toSquare, &plane->GetNormal());

	//if(fDotResult < fRadius)
	//	return true;

	return false;
}

bool Square::WithSquare(const Square* square)
{
#if _WIN32
	if(_position > square->m_Max || m_Max < square->_position)
#else
	if (glm::all(glm::greaterThan(_position, square->m_Max)) && glm::all(glm::lessThan(m_Max, square->_position)))
#endif
		return false;
	
	return true;
}

bool Square::Check(vector2 point)
{
#if _WIN32
	if (point > _position && point < m_Max)
#else
	// I wish GLM just overrode the comparison operators
	if (glm::all(glm::greaterThan(point, _position)) && glm::all(glm::lessThan(point, m_Max)))
#endif
      return true;

   return false;
}

// This function is likely to look the same for all of these objects...
bool Square::Check(const Collidable* collidable)
{
   switch (collidable->getType()) {
   case COL_OBJ_SQUARE:
      return this->WithSquare((Square*)collidable);
   default:
      return false;
   }
}