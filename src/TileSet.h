// TileSet.h
#pragma once

#include <map>

#ifndef _TILESET_H_
#define _TILESET_H_

struct TileSet
{
   Texture*  _tileSheet;

   unsigned int _tileSize;  // Square tiles only for now

   // Meta-information about tilesIndices (what does a particular tile in a tileSheet /mean/?)
   // This makes sense only when a tile is with other tyles
   std::map<unsigned int, std::string> _tileInfo; 
};

#endif