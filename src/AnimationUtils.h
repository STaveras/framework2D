#pragma once

#include "Types.h"
#include "IRenderer.h"

#include <vector>

class Animation;
class AnimationManager;

#include "ANIMATION_MODE.h"

namespace Animations
{
   // If not using an AnimationManager, caller must manually delete memory (Sprites, Frames, and Animations themselves)
   std::vector<Animation*> fromXML(const char *filename, AnimationManager *manager = NULL);
   
   void addToRenderList(std::vector<Animation*>& animations, IRenderer::RenderList *renderList);
   void destroyAnimation(Animation *animation);
}
