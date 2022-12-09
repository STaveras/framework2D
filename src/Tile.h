#pragma once

#include "GameObject.h"

#include "TileSet.h"
#include "Square.h"

#include <algorithm>
#include <vector>

// Just to get something on the screen...
class Tile : public GameObject
{
   int _tileIndex = -1; // How far in the tileSheet this block is

   TileSet* _tileSet = NULL;

   // TODO: Move this to TileSet
   Factory<Image> _tileImages;

public:
   Tile(void) : GameObject(GAME_OBJ_TILE) {
      this->addState("");
      this->start();
   }

   // tileIndex which tile to use, starting from 0, left-to-right, top-to-bottom
   Tile(int tileIndex, TileSet* tileSet) :
      GameObject(GAME_OBJ_TILE),
      _tileSet(tileSet) {

      if (_tileSet) {
         this->addState("Static"); // Look up the tilename in the tile set info
         this->start();
         this->setTileIndex(tileIndex);
      }
   }

   ~Tile(void) {

      GameObjectState* state = this->getState();
      Collidable* collidable = state->getCollidable();

      if (collidable) {
         delete collidable;
      }

      _tileImages.clear();
   }

   int getTileIndex(void) const {
      return _tileIndex;
   }

   TileSet* getTileSet(void) const {
      return _tileSet;
   }

   void setTileIndex(int tileIndex) 
   {
      _tileIndex = tileIndex;

      if (_tileSet && _tileIndex >= 0) {

         if (_tileIndex < _tileSet->getTileCounts().x * _tileSet->getTileCounts().y) {

            GameObjectState* state = this->getState();

            if (state) {

               UINT xPosition = (_tileIndex % (UINT)_tileSet->getTileCounts().x) * _tileSet->getTileSize();
               UINT yPosition = (_tileIndex / (UINT)_tileSet->getTileCounts().x) * _tileSet->getTileSize();

               UINT width = xPosition + _tileSet->getTileSize();
               UINT height = yPosition + _tileSet->getTileSize();

               RECT tileRect{
                  (int)xPosition, (int)yPosition, (int)width, (int)height
               };

               Image* tileImage = (Image*)state->getRenderable();

               if (tileImage) {
                  _tileImages.destroy(tileImage);
               }
               else {
                  tileImage = _tileImages.create();
                  tileImage->setSourceRect(tileRect);
                  tileImage->setTexture(_tileSet->getTileSheet());
               }

               state->setRenderable(tileImage);

               Square* tileCollidable = (Square*)state->getCollidable();

               if (tileCollidable) {
                  delete state->getCollidable();
               }
               else {
                  tileCollidable = new Square(vector2(0, 0), (float)_tileSet->getTileSize(), (float)_tileSet->getTileSize());
                  state->setCollidable(tileCollidable);
               }
            }
         }
      }
   }

   void setTileSet(TileSet* tileSet) {

      _tileSet = tileSet;

      if (_tileSet) {
         this->setTileIndex(_tileIndex);
      }
   }
};
