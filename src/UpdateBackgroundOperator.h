#pragma once

#include "Background.h"
#include "ObjectOperator.h"
#include "Renderable.h"

class Camera;

// This is probably gonna be one of the crazier operators I'll make

class UpdateBackgroundOperator: public ObjectOperator
{
   Background::Mode _mode;
   Image* _background;
   Camera* _camera; // for checking

   std::list<Image*> _cache; // Stores the extra images for other modes

   IRenderer::RenderList* _renderList;

public:

   void setMode(Background::Mode mode) { _mode = mode; }
   void setBackground(Image *image) { _background = image; }

   void useRenderList(RenderList *renderList) { _renderList = renderList; }

   void clearCache(void) {

      while (!_cache.empty()) {
         delete _cache.back();
         _cache.pop_back();
      }

   }

   bool operator()(GameObject *object) {

      switch (_mode) {
      case Background::Mode_Repeat:

         break;
      case Background::Mode_Mirror:



         break;
      default: // Background::Mode_Still:
         break; // Do nothing, really
      }
      return true;
   }
};