// File: Square.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "Square.h"
#include "Plane.h"
#include "Circle.h"

bool WithCircle(const Square* square, const Circle* circle)
{
   float left = square->getMin().x;
   float right = square->getMax().x;
   float top = square->getMin().y;
   float bottom = square->getMax().y;

   // Just to make things a bit more readable later on...
   float cx = circle->_x;
   float cy = circle->_y;
   float radius = circle->getRadius();

   //int midx = (int)Paddle.m_Properties.m_Position.x;
   //int midy = (int)Paddle.m_Properties.m_Position.y;

   vector2 p[4] =
   {
       vector2(left,top),
       vector2(right,top),
       vector2(left,bottom),
       vector2(right,bottom)
   };

   vector2 axis[6];

   vector2 center(cx, cy);

   int i;
   for (i = 0; i < 4; i++)
   {
      axis[i] = ((vector2)(p[i] - center)).normalize();
      p[i] = p[i] - center;
   }

   axis[4] = vector2(1, 0);
   axis[5] = vector2(0, 1);

   //for (i = 0; i < 6; i++)
   //{
   //   float min = INT_MAX;
   //   float max = -min;
   //   for (int j = 0; j < 4; j++)
   //   {
   //      float project = (p[j] * axis[i]);
   //      if (project < min)
   //         min = project;
   //      if (project > max)
   //         max = project;
   //   }

   //   if (min > radius || max < -radius)
   //      return false; // found axis of separation
   //}

   //// Check the distance between the ball's center and the closest point on the paddle's bounding box
   //vector2 closest(cx, cy);
   //closest.x = std::max(left, std::min(cx, right));
   //closest.y = std::max(top, std::min(cy, bottom));
   //if (((vector2)(closest - center)).length() > radius)
   //   return false;

   return true; // no axis of separation -> intersection
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
   case COL_OBJ_CIRCLE:
      return WithCircle(this, (Circle*)collidable);
   default:
		return this->collidesWith(collidable->getPosition());
   }
}