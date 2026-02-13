// TileSet.h
#pragma once

#include "ITexture.h"
#include "Collidable.h"

#include <map>
#include <string>
#include <unordered_map>

#ifndef _TILESET_H_
#define _TILESET_H_

class TileSet
{
   Texture*  _tileSheet;

   unsigned int _tileSize;  // Square tiles only for now

   vector2 _tileCounts;

public:
   struct CollisionLoadStats {
      int explicitColliders = 0;
      int missingColliders = 0;
      int decomposedConcavePolygons = 0;
   };

   struct TileInfo {
      std::string _typeName;
      Collidable* _collisionInfo = NULL;
      std::unordered_map<std::string, std::string> _properties;
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

   const std::unordered_map<std::string, std::string>* getTileProperties(int tileIndex) const {
      std::map<int, TileInfo>::const_iterator itr = _tileInfo.find(tileIndex);
      if (itr == _tileInfo.end()) {
         return NULL;
      }
      return &(itr->second._properties);
   }

   static TileSet* loadFromFile(const char* fileName);
   static CollisionLoadStats getCollisionLoadStats(void);
   static void resetCollisionLoadStats(void);
};

#endif
