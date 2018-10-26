#pragma once

#include "Types.h"

#include <vector>

class Animation;
class AnimationManager;

#include "ANIMATION_MODE.h"

namespace AnimationUtils
{
   // NOTE: Maybe move this elsewhere...?
   RECT rectFromString(const char *rectDescription);
   ANIMATION_MODE animationModeFromString(const char *mode);

   // If not using an AnimationManager, caller must manually delete memory (Sprites, Frames, and Animations themselves)
   std::vector<Animation*> loadAnimationsFromXML(const char *filename, AnimationManager *manager = NULL);
 }
