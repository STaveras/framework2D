// TileSet.h
#pragma once

#include "ITexture.h"
#include "Collidable.h"

#include <map>
#include <string>
#include <utility>

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
   };

   // Tiled stores a tile's flip/rotation in the top three bits of its gid. A 90 degree
   // rotation is a diagonal flip combined with a horizontal (clockwise) or vertical flip.
   static constexpr unsigned int kFlipHorizontal = 0x80000000u;
   static constexpr unsigned int kFlipVertical = 0x40000000u;
   static constexpr unsigned int kFlipDiagonal = 0x20000000u;
   static constexpr unsigned int kFlipMask = kFlipHorizontal | kFlipVertical | kFlipDiagonal;

   // Maps a point inside a tile (0..size on both axes) through the flip flags, in
   // Tiled's order: diagonal first, then horizontal, then vertical.
   static vector2 flipTilePoint(vector2 point, float size, unsigned int flipFlags);

   //static Factory<TileSet> _globalTileSets;

protected:
   // Meta-information about tilesIndices (what does a particular tile in a tileSheet /mean/?)
   // This makes sense only when a tile is with other tyles
   std::map<int, TileInfo> _tileInfo; 

   // Flipped copies of tile colliders, created on first use per (tile, flip flags).
   std::map<std::pair<int, unsigned int>, Collidable*> _flippedCollision;

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

   // The tile's collider with the given flip flags applied; NULL when the tile has none.
   Collidable* getCollision(int tileIndex, unsigned int flipFlags = 0);

   static TileSet* loadFromFile(const char* fileName);
   static CollisionLoadStats getCollisionLoadStats(void);
   static void resetCollisionLoadStats(void);
};

#endif
