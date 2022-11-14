// File: CollidableGroup.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010
#pragma once
#if !defined(_COLLISIONGROUP_H)
#define _COLLISIONGROUP_H
#include "Collidable.h"
#include <list>
class CollidableGroup : public Collidable, public std::list<Collidable*>
{
public:
	CollidableGroup(void):Collidable(COL_OBJ_GROUP){}

	bool collidesWith(const Collidable* colObj);
};

#endif  //_COLLISIONGROUP_H