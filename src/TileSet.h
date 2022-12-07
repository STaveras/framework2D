// TileSet.h
#pragma once

#include "ITexture.h"

#include <map>
#include <string>

#ifndef _TILESET_H_
#define _TILESET_H_

class TileSet
{
   Texture*  _tileSheet;

   unsigned int _tileSize;  // Square tiles only for now

   vector2 _tileCounts;

   // Meta-information about tilesIndices (what does a particular tile in a tileSheet /mean/?)
   // This makes sense only when a tile is with other tyles
   std::map<unsigned int, std::string> _tileInfo; 

public:
   explicit TileSet(Texture* tileSheet, unsigned int tileSize) :
      _tileSheet(tileSheet),
      _tileSize(tileSize) {

      _tileCounts = { (float)(tileSheet->getWidth() / tileSize), (float)(tileSheet->getHeight() / tileSize) };
   }

   unsigned int getTileSize(void) const { return _tileSize; }
   vector2 getTileCounts(void) const { return _tileCounts; }
   Texture* getTileSheet(void) const { return _tileSheet; }
};

#endif