// TileSet.h
#pragma once

#include "ITexture.h"
#include "Collidable.h"

#include <map>
#include <string>

#ifndef _TILESET_H_
#define _TILESET_H_

class TileSet
{
   Texture*  _tileSheet;

   unsigned int _tileSize;  // Square tiles only for now

   vector2 _tileCounts;

public:
   struct TileInfo {
      std::string _typeName;
      Collidable* _collisionInfo = NULL;
   };

   //static Factory<TileSet> _globalTileSets;

protected:
   // Meta-information about tilesIndices (what does a particular tile in a tileSheet /mean/?)
   // This makes sense only when a tile is with other tyles
   std::map<int, TileInfo> _tileInfo; 

public:
   explicit TileSet(Texture* tileSheet, unsigned int tileSize) :
      _tileSheet(tileSheet),
      _tileSize(tileSize) {

      if (_tileSheet) {
         _tileCounts = { (float)(tileSheet->getWidth() / tileSize), (float)(tileSheet->getHeight() / tileSize) };
      }
   }

   float getTileSize(void) const { return (float)_tileSize; }
   vector2 getTileCounts(void) const { return _tileCounts; }
   Texture* getTileSheet(void) const { return _tileSheet; }

   TileInfo getTileInfo(int tileIndex) {
      return _tileInfo[tileIndex];
   }

   static TileSet* loadFromFile(const char* fileName);
};

#endif