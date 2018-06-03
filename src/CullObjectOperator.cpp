// File: CullObjectOperator.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "CullObjectOperator.h"

bool CullObjectOperator::operator()(GameObject* object)
{
   // NOTE: Remembering now... The intent was to use the operator's return to skip updating the object in ObjectManager;
   //       But that wouldn't work correctly since it would mean that if an object is off screen, it won't get updated..
   //       That's dumb. I'm was dumb. In some respects, it seems. All we should be wanting to do is not render the obj.
	return false;
}

