// TileMap.h
#pragma once

#include "Tile.h"

#ifndef _TILEMAP_H_
#define _TILEMAP_H_

class TileMap : public Tile
{
	unsigned int** _tileGrid = nullptr;
	unsigned int _mapWidth;
	unsigned int _mapHeight;

	TileSet* _tileSet;
	Factory<Tile> _tiles;

public:
	explicit TileMap(unsigned int mapWidth, unsigned int mapHeight, TileSet* tileSet) :
		_tileSet(tileSet),
		_mapWidth(mapWidth),
		_mapHeight(mapHeight) {
		_tileGrid = new unsigned int* [mapWidth];
		for (unsigned int i = 0; i < mapWidth; i++) {
			_tileGrid[i] = new unsigned int[mapHeight];
		}
		memset(_tileGrid, 0, mapWidth * mapHeight * sizeof(unsigned int));
	}

	virtual ~TileMap(void) {
		for (unsigned int i = 0; i < _mapWidth; i++) {
			delete[] _tileGrid[i];
		}
		delete[] _tileGrid;
	}

	void setTileIndex(unsigned int x, unsigned int y, unsigned int tileIndex) {
		_tileGrid[x][y] = tileIndex;
	}

	unsigned int getTileIndex(unsigned int x, unsigned int y) const {
		return _tileGrid[x][y];
	}

	unsigned int getMapWidth(void) const {
		return _mapWidth;
	}

	unsigned int getMapHeight(void) const {
		return _mapHeight;
	}

	Tile* getTile(unsigned int x, unsigned int y) {

		Tile* tile = NULL;

		if (_tileSet) {

			if (x < _mapWidth && y < _mapHeight) {

				if (x * y < _tiles.size()) {
					return _tiles[_mapWidth * x + y];
				}
				else {
					unsigned int tileSize = _tileSet->getTileSize();

					tile = _tiles.create();
					tile->setTileSet(_tileSet);
					tile->setTileIndex(_tileGrid[x][y]);
					tile->setPosition({ (float)x * tileSize, (float)y * tileSize });
				}
			}
		}

		return tile;
	}

	void arrangeTiles(void) {
		for (unsigned int x = 0; x < _mapWidth; x++) {
			for (unsigned int y = 0; y < _mapHeight; y++) {
				Tile* tile = getTile(x, y);
				if (tile) {
					unsigned int tileSize = tile->getTileSet()->getTileSize();
					tile->setPosition({ (float)x * tileSize, (float)y * tileSize });
				}
			}
		}
	}

	// static TileMap* loadFromFile(const char* filePath) {

	//    TileMap* tileMap = nullptr;

	//    return tileMap;
	// }
};

#endif