#pragma once

#include "Types.h"

#include <map>

class Animation;
class AnimationManager;

class AnimationLoader
{
public:
   AnimationLoader();

   static std::map<std::string, Animation*> loadAnimationsFromXML(const char *filename, AnimationManager *manager = NULL);

   virtual ~AnimationLoader() = 0;
};
