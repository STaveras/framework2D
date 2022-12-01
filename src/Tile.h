#pragma once

#include "GameObject.h"

#include <algorithm>
#include <vector>

//class Tile;
//
//class TileSet
//{
//
//};

// Just to get something on the screen...
class Tile : public GameObject
{
   std::string _type;

   Texture* _tileSheet;

   unsigned int _tileIndex; // How far in the tileSheet this block is
   unsigned int _tileSize;  // Square tiles only for now

   vector2 _tileCounts;

   Factory<Image> _tileImages;


public:

   std::string getType(void) const { return _type; }

   // tileSize size in pixels // squared 
   // tileIndex which tile to use, starting from 0, left-to-right, top-to-bottom
   // tileType string which describes the tile; doesn't have to be human readable
   Tile(Texture* tileSheet, unsigned int tileSize, unsigned int tileIndex, std::string tileType) :
      GameObject(GAME_OBJ_OBJECT),
      _tileSheet(tileSheet),
      _tileIndex(tileIndex),
      _tileSize(tileSize),
      _type() {

      if (_tileSheet) {

         _tileCounts = { (float)(tileSheet->getWidth() / tileSize), (float)(tileSheet->getHeight() / tileSize) };

         if (tileIndex < _tileCounts.x * _tileCounts.y) {

            UINT xPosition = (tileIndex % (UINT)_tileCounts.x) * tileSize;
            UINT yPosition = (tileIndex / (UINT)_tileCounts.x) * tileSize;

            UINT width = xPosition + tileSize;
            UINT height = yPosition + tileSize;

            RECT tileRect{
               (int)xPosition, (int)yPosition, (int)width, (int)height
            };

            Image* tileImage = _tileImages.create();
            tileImage->setSourceRect(tileRect);
            tileImage->setTexture(tileSheet);

            ((GameObjectState*)this->addState("Static"))->setRenderable(tileImage);
         }
      }
   }

   ~Tile(void) {
      _tileImages.clear();
   }
};