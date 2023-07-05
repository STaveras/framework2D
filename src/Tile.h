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
      this->setStatic(true);
   }

   // tileIndex which tile to use, starting from 0, left-to-right, top-to-bottom
   explicit Tile(int tileIndex, TileSet* tileSet) :
      GameObject(GAME_OBJ_TILE),
      _tileSet(tileSet) {

      if (_tileSet) {
         this->addState(_tileSet->getTileInfo(tileIndex)._typeName.c_str()); // Look up the tileType in the tile set info
         this->start();
         this->setTileIndex(tileIndex);
         this->setStatic(true);
      }
   }

   ~Tile(void) {
      _tileImages.clear();
   }

   TileSet* getTileSet(void) const {
      return _tileSet;
   }

   int getTileIndex(void) const {
      return _tileIndex;
   }

   std::string getTileType(void) const {
      return _tileSet->getTileInfo(_tileIndex)._typeName;
   }

   void setTileIndex(int tileIndex) 
   {
      _tileIndex = tileIndex;

      if (_tileSet && _tileIndex >= 0) {

         if (_tileIndex < _tileSet->getTileCounts().x * _tileSet->getTileCounts().y) {

            GameObjectState* state = this->getState();

            if (state) {

               int xPosition = (int)((_tileIndex % (int)_tileSet->getTileCounts().x) * _tileSet->getTileSize());
               int yPosition = (int)((_tileIndex / (int)_tileSet->getTileCounts().x) * _tileSet->getTileSize());

               int width = (int)(xPosition + _tileSet->getTileSize());
               int height = (int)(yPosition + _tileSet->getTileSize());

               RECT tileRect{
                  xPosition, yPosition, width, height
               };

               Image* tileImage = (Image*)state->getRenderable();

               if (tileImage) {
                  _tileImages.destroy(tileImage);
               }
               else {
                  tileImage = _tileImages.create();
                  tileImage->setSrcRect(tileRect);
                  tileImage->setTexture(_tileSet->getTileSheet());
               }

               state->setRenderable(tileImage);

               TileSet::TileInfo tileInfo = _tileSet->getTileInfo(tileIndex);

               if (tileInfo._typeName != "") {
                  state->setName(tileInfo._typeName.c_str());
               }

               if (tileInfo._collisionInfo != NULL) {
                  state->setCollidable(_tileSet->getTileInfo(tileIndex)._collisionInfo);
               }
               // Originally we created new Collision objects here... But now, we entirely rely on the TileSet for these objects
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

#ifdef _DEBUG
   void update(float time) {
      GameObject::update(time);

      if (Debug::dbgTiles) 
      {
         char buffer[128]{ 0 };
         sprintf_s(buffer, 128, "pos{%f, %f}\n", this->getPosition().x, this->getPosition().y);
         DEBUG_MSG(buffer);

         if (Collidable* collidable = this->getCollidable()) {
            sprintf_s(buffer, 128, "cpos{%f, %f}\n", this->getCollidable()->getPosition().x, this->getCollidable()->getPosition().y);
            DEBUG_MSG(buffer);
         }

         DEBUG_MSG("\n");
      }
   }
#endif
};
