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

   void _updateCache() {

      if (_background) {

         short flip = 1;
         short axis = 0;
         short count = 0;

         vector2 lastScale = _background->getScale();
         vector2 lastPos = _background->getPosition();

         // We need a better way to make a spiral
         // TODO: Needs to be a generalized math function to make it any size, to support zooming (just cus)
         for (int i = 0; i < 8; i++) {

            if (_firstRun) {
               _cache.push_back(new Image(*_background));

               if (_renderList)
                  _renderList->push_front(_cache.back());
            }

            // Ugh I hate the below so bad but my brain isn't working as good as it used to... D:
            if (i < 2) 
            {
               updatePos(lastPos, axis++, flip);

               if (axis > 1)
               {
                  axis = 0;
                  flip = -flip;
               }
            }
            else {

               if (i == 5)
                  flip = -flip;

               updatePos(lastPos, axis, flip);
               
               if (i % 2 == 1)
                  axis = !axis;
            }

            _cache.back()->setPosition(lastPos);

            if (_mode == Background::Mode_Mirror) {
               lastScale.x = (axis == 1) ? -lastScale.x : lastScale.x;
               lastScale.y = (axis == 0) ? -lastScale.y : lastScale.y;
            }
            
            _cache.back()->setScale(lastScale);

            //_cache.back()->center();
         }

         _firstRun = false;
      }
   }

   void updatePos(D3DXVECTOR2 &lastPos, short axis, short flip)
   {
      lastPos.x += (axis == 0 ? flip * _background->getWidth() : 0);
      lastPos.y += (axis == 1 ? flip * _background->getHeight() : 0);
   }

public:

   ~UpdateBackgroundOperator() {
      clearCache();
   }

   void setMode(Background::Mode mode) { _mode = mode; }
   void setBackground(Image *image) { _background = image; _firstRun = true; }
   void setCamera(Camera *camera) { _camera = camera; }

   void useRenderList(IRenderer::RenderList *renderList) { _renderList = renderList; clearCache(); }

   void clearCache(void) {

      while (!_cache.empty()) {
         _renderList->remove(_cache.back());
         delete _cache.back();
         _cache.pop_back();
      }

      _firstRun = true;

   }

   bool operator()(GameObject *object) {

      _updateCache();

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