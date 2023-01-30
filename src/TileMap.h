// TileMap.h
#pragma once

// TODO: Create a collidable encompassing the map

#include "Tile.h"
#include "StrUtil.h"
#include "FileSystem.h"

#ifndef _TILEMAP_H_
#define _TILEMAP_H_

class TileMap : public Tile
{
	unsigned int _mapWidth;
	unsigned int _mapHeight;

	TileSet* _tileSet;
	Factory<Tile> _tiles;

public:
	explicit TileMap(unsigned int mapWidth, unsigned int mapHeight, TileSet* tileSet) :
		_tileSet(tileSet),
		_mapWidth(mapWidth),
		_mapHeight(mapHeight) {

		if (_tileSet) {
			for (unsigned int i = 0; i < mapWidth; i++) {
				for (unsigned int j = 0; j < mapHeight; j++) {
					_tiles.create()->setTileSet(_tileSet);
				}
			}
		}
	}

	virtual ~TileMap(void) {
		_tiles.clear();
	}

	void setTileIndex(unsigned int x, unsigned int y, int tileIndex) {
		this->getTile(x, y)->setTileIndex(tileIndex);
	}

	int getTileIndex(unsigned int x, unsigned int y) {
		return this->getTile(x, y)->getTileIndex();
	}

	unsigned int getMapWidth(void) const {
		return _mapWidth;
	}

	unsigned int getMapHeight(void) const {
		return _mapHeight;
	}

	Factory<Tile>& getTiles(void) { return _tiles; }

	Tile* getTile(unsigned int x, unsigned int y) {
		
		if (_tileSet && _mapWidth > x && y < _mapHeight && x * y <= _tiles.size()) {
			return _tiles[x + y * _mapWidth];
		}

		return NULL;
	}

	void setMapWidth(unsigned int mapWidth) { _mapWidth = mapWidth; }
	void setMapHeight(unsigned int mapHeight) { _mapHeight = mapHeight; }

	void arrangeTiles(void) {
		for (unsigned int x = 0; x < _mapWidth; x++) {
			for (unsigned int y = 0; y < _mapHeight; y++) {
				if (Tile* tile = getTile(x, y)) {
					unsigned int tileSize = (unsigned int)tile->getTileSet()->getTileSize();
					tile->setPosition(this->getPosition() + vector2{ round((float)(x * tileSize)), round((float)(y * tileSize)) });
					tile->update(0);
#if _DEBUG
					if (Debug::dbgTiles) {
						if (Collidable* collidable = tile->getCollidable()) {
							char buffer[128]; sprintf_s(buffer, "pos{%f, %f}\tcpos{%f, %f}\n", tile->_x, tile->_y, collidable->_position.x, collidable->_position.y);
							DEBUG_MSG(buffer);
						}
					}
#endif
				}
			}
		}
	}

	static TileMap* loadFromCSVFile(const char* filePath, TileSet* tileSet) {

	   TileMap* tileMap = nullptr;

	   std::ifstream file(filePath);

	   if (file.is_open()) {

			std::vector<std::vector<int>> lines;

			do {
				std::string line;
				std::getline(file, line);

				if (!line.empty()) {

					std::vector<int> indices;
					std::vector<std::string> tokens = split(line, ',');

					if (!tokens.empty()) {

						for (std::string index : tokens) {
							indices.push_back(std::stoi(index));
						}

						lines.push_back(indices);
					}
				}
			}
			while(file.good());

			unsigned int mapHeight = (unsigned int)lines.size();
			unsigned int mapWidth = (unsigned int)lines[0].size();

			if (tileSet) {
				tileMap = new TileMap(mapWidth, mapHeight, tileSet);

				for (unsigned int y = 0; y < mapHeight; y++) {
					for (unsigned int x = 0; x < mapWidth; x++) {
						tileMap->setTileIndex(x, y, lines[y][x]);
					}
				}

				tileMap->arrangeTiles();
			}			
	   }

		file.clear();
		file.close();

	   return tileMap;
	}
};

#endif