#pragma once

#include "GameObject.h"

// Just to get something on the screen...
class Tile : public GameObject
{
   Image* _tileSheet;
   unsigned int _tileCount; // How many tiles we can split this image into
   unsigned int _tileIndex; // How far in the tileSheet this block is
   unsigned int _tileSize;  // Square tiles only for now

   std::string _type[];

public:

   std::string GetType(void) const { return _type[_tileIndex]; }

   unsigned int GetTileCount(void) {

      if (_tileCount == -1) {// Obviously won't work on 64 so ya...

         unsigned int tileCountX = (_tileSheet->getWidth() / _tileSize);
         unsigned int tileCountY = (_tileSheet->getHeight() / _tileSize);

         _tileCount = tileCountX * tileCountY;
      }

      return _tileCount;
   }

   Tile(Image* tileSheet, unsigned int tileSize, unsigned int tileIndex, std::string tileType) :
      GameObject(GAME_OBJ_OBJECT),
      _tileSheet(tileSheet),
      _tileIndex(tileIndex),
      _tileSize(tileSize),
      _type(),
      _tileCount(-1) {

      if (tileSheet) {

         for (unsigned int i = _tileIndex; i < _tileCount; i++) {

            //           struct {
                          //LONG left = ;
                          //LONG top;
                          //LONG right;
                          //LONG bottom;
            //           }rect;

            RECT tileRect{
               i * this->_tileSize,

            };




            _tileCount++; // :)
         }






      }
   }

   ~Tile(void) {

   }
};