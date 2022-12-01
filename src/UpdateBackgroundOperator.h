#pragma once

#include "Background.h"
#include "ObjectOperator.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Factory.h" // should maybe use this for the cache...? 

#include <vector>

class Camera;

// NOTES: 
// So originally I wanted this to make an infinite tile grid of background images at any scale, 
// but also being memory-conscious enough to clean the things off-screen. The idea is to build a
// cache of images, based on the original one you give it. However my brain no longer works, so 
// I keep getting distracted, going through multiple iterations, etc. etc. and I'm just going to
// make a simple version where it just mirrors in one direction as you go along, and no cleanup...

class UpdateBackgroundOperator: public ObjectOperator
{
   IRenderer::RenderList* _renderList;

   bool _firstRun = true;

   Image* _background;
   Camera* _camera; 

   std::vector<Image*> _cache; // Stores the extra images for other modes

   Background::Mode _mode;

   //void _updatePos(vector2& lastPos, short axis, short flip);
   void _resetCache(void);
   void _updateCache(void);

public:

   ~UpdateBackgroundOperator() {
      _resetCache();
   }

   Image* getBackground(void) const { return _background; }

   void setMode(Background::Mode mode) { _mode = mode; }
   void setBackground(Image *image) { _background = image; _resetCache(); }
   void setCamera(Camera *camera) { _camera = camera; }

   void useRenderList(IRenderer::RenderList *renderList) { _renderList = renderList; _resetCache(); }

   bool operator()(GameObject* object);

};