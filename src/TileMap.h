// TileMap.h
#pragma once

#include "Tile.h"
#include "StrUtil.h"
#include "FileSystem.h"

#ifndef _TILEMAP_H_
#define _TILEMAP_H_

class TileMap : public Tile
{
	//int** _tileGrid = nullptr;
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

			//_tileGrid = new int* [mapWidth];

			for (unsigned int i = 0; i < mapWidth; i++) {

				//_tileGrid[i] = new int[mapHeight];

				for (unsigned int j = 0; j < mapHeight; j++) {

					//_tileGrid[i][j] = -1;

					unsigned int tileSize = _tileSet->getTileSize();

					Tile* tile = _tiles.create();
					tile->setTileSet(_tileSet);
					tile->setPosition(this->getPosition() + vector2{ (float)i * tileSize, (float)j * tileSize });
				}
			}
		}
	}

	virtual ~TileMap(void) {
		_tiles.clear();
		//if (_tileGrid) {
		//	for (unsigned int i = 0; i < _mapWidth; i++) {
		//		delete[] _tileGrid[i];
		//	}
		//	delete[] _tileGrid;
		//}
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

	void arrangeTiles(void) {
		for (unsigned int x = 0; x < _mapWidth; x++) {
			for (unsigned int y = 0; y < _mapHeight; y++) {
				Tile* tile = getTile(x, y);
				if (tile) {
					unsigned int tileSize = tile->getTileSet()->getTileSize();
					tile->setPosition(this->getPosition() + vector2{ (float)x * tileSize, (float)y * tileSize });
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
				// Read the first line to get the TileMap width
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

			unsigned int mapHeight = lines.size();
			unsigned int mapWidth = lines[0].size();

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