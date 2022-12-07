#pragma once

#include "GameObject.h"

#include "TileSet.h"

#include <algorithm>
#include <vector>

// Just to get something on the screen...
class Tile : public GameObject
{
   unsigned int _tileIndex = UINT_MAX; // How far in the tileSheet this block is

   TileSet* _tileSet = NULL;

   // TODO: Move this to TileSet
   Factory<Image> _tileImages;

public:

   Tile(void) : GameObject(GAME_OBJ_TILE) {
      this->addState("");
      this->start();
   }

   // tileIndex which tile to use, starting from 0, left-to-right, top-to-bottom
   Tile(unsigned int tileIndex, TileSet* tileSet) :
      GameObject(GAME_OBJ_TILE),
      _tileSet(tileSet) {

      if (_tileSet) {
         this->addState("Static"); // Look up the tilename in the tile set
         this->start();
         this->setTileIndex(tileIndex);
      }
   }

   ~Tile(void) {
      _tileImages.clear();
   }

   unsigned int getTileIndex(void) const {
      return _tileIndex;
   }

   TileSet* getTileSet(void) const {
      return _tileSet;
   }

   void setTileIndex(unsigned int tileIndex) 
   {
      _tileIndex = tileIndex;

      if (_tileSet) {

         if (tileIndex < _tileSet->getTileCounts().x * _tileSet->getTileCounts().y) {

            UINT xPosition = (tileIndex % (UINT)_tileSet->getTileCounts().x) * _tileSet->getTileSize();
            UINT yPosition = (tileIndex / (UINT)_tileSet->getTileCounts().x) * _tileSet->getTileSize();

            UINT width = xPosition + _tileSet->getTileSize();
            UINT height = yPosition + _tileSet->getTileSize();

            RECT tileRect{
               xPosition, yPosition, width, height
            };

            Image* tileImage = _tileImages.create();
            tileImage->setSourceRect(tileRect);
            tileImage->setTexture(_tileSet->getTileSheet());

            ((GameObjectState*)this->getState())->setRenderable(tileImage);
         }
      }
   }

   void setTileSet(TileSet* tileSet) {

      _tileSet = tileSet;

      if (_tileSet) {

         Image* image = (Image*)this->getState()->getRenderable();

         if (image) {

            _tileImages.destroy(image);
            this->setTileIndex(_tileIndex);

         }
      }
   }
};
