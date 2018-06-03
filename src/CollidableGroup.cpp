// File: CollidableGroup.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010
#include "CollidableGroup.h"

bool CollidableGroup::Check(const Collidable* colObj)
{
   std::list<Collidable*>::iterator itr = this->begin();

   while (itr != this->end()) {

      if ((*itr)->Check(colObj))
         return true;

      itr++;
   }

   return false;
}
