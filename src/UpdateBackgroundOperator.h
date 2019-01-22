#pragma once

#include "Background.h"
#include "ObjectOperator.h"
#include "Renderer.h"
#include "Image.h"
#include "Factory.h" // should maybe use this for the cache...? 

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

   std::list<Image*> _cache; // Stores the extra images for other modes

   Background::Mode _mode;

   void _updateCache() {

      if (_background) {

         short flip = 1;
         short axis = 0;
         short count = 0;

         vector2 lastPos = _background->getPosition();

         // We need a better way to make a spiral
         // TODO: Needs to be a generalized math function to make it any size, to support zooming (just cus)
         for (int i = 0; i < 8; i++) {

            if (_firstRun) {
               _cache.push_back(new Image(*_background));

               if (_renderList)
                  _renderList->push_front(_cache.back());

               _cache.back()->center();
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

   Image* getBackground(void) const { return _background; }

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

      //_updateCache();

      //switch (_mode) {
      //case Background::Mode_Repeat:



      //   break;
      //case Background::Mode_Mirror:


      //   break;
      //default: // Background::Mode_Still:
      //   break; // Do nothing, really
      //}
      return true;
   }
};