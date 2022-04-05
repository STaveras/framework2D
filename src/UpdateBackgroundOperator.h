#pragma once

#include "Background.h"
#include "ObjectOperator.h"
#include "Renderer.h"
#include "Image.h"
#include "Factory.h" // should maybe use this for the cache...? 

class Camera;

// This is probably gonna be one of the crazier operators I'll make

class UpdateBackgroundOperator: public ObjectOperator
{
   IRenderer::RenderList* _renderList;

   bool _firstRun = true;

   Image* _background;
   Camera* _camera; 

   std::list<Image*> _cache; // Stores the extra images for other modes

   Background::Mode _mode;

   void _updatePos(vector2& lastPos, short axis, short flip);
   void _clearCache(void);
   void _updateCache(void);

public:

   ~UpdateBackgroundOperator() {
      _clearCache();
   }

   void setMode(Background::Mode mode) { _mode = mode; }
   void setBackground(Image *image) { _background = image; _clearCache(); }
   void setCamera(Camera *camera) { _camera = camera; }

   void useRenderList(IRenderer::RenderList *renderList) { _renderList = renderList; _clearCache(); }

   bool operator()(GameObject *object) {

      _updateCache();

      switch (_mode) {
      case Background::Mode::Repeat:
         break;
      case Background::Mode::Mirror:
          break;
      default: // Background::Mode::Still:
         break; // Do nothing, really
      }
      return true;
   }
};