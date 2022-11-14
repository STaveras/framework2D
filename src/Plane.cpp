// File: Plane.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "Plane.h"

//bool WithCircle(const Circle* circle);
//bool WithPlane(const Plane* plane);
//bool WithSquare(const Square* square);

bool Plane::collidesWith(const Collidable* c) {

	 switch (c->getType()) {
	 case COL_OBJ_PLANE:


		  break;

	 }

	 return false;
}