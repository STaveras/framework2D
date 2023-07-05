#pragma once

#include "Types.h"
#include "IRenderer.h"

#include "Sprite.h"

#include <vector>

class Animation;
class AnimationManager;

namespace Animations
{
   // If not using an AnimationManager, caller must manually delete memory (Sprites, Frames, and Animations themselves)
   std::vector<Animation*> fromXML(const char *filename, AnimationManager *manager = NULL);
   std::vector<Animation*> fromJSON(const char* filename, AnimationManager* manager);
   std::vector<Animation*> fromDirectory(const char* directoryPath);

   void toJSON(const std::vector<Animation*>& animations, const char* filename);
   
   void addToRenderList(std::vector<Animation*>& animations, IRenderer::RenderList *renderList);
   void destroyAnimation(Animation *animation);

   // Only works when each Frame have the same dimensions
   void createFramesForAnimation(Animation* animation, 
                                 Texture* spriteSheet, 
                              vector2 frameDimensions, 
                       Factory<Sprite>& spriteFactory,
                              unsigned int startIndex = 0, 
                              unsigned int count = 0);
}
