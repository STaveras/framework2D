// File: CollidableGroup.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010
#if !defined(_COLLISIONGROUP_H)
#define _COLLISIONGROUP_H
#include "Collidable.h"
#include <list>
class CollidableGroup : public Collidable
{
	std::list<Collidable*> m_lsCollisionObjects;

public:
	CollidableGroup(void):Collidable(COL_OBJ_GROUP){}

	bool CheckCollision(const Collidable* colObj);
	void Push(const Collidable* colObject);
	void Pop();
	void Clear();
};

#endif  //_COLLISIONGROUP_H