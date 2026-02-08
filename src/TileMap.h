// TileMap.h
#pragma once

// TODO: Create a collidable encompassing the map

#include "Tile.h"
#include "StrUtil.h"
#include "FileSystem.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>

#ifndef _TILEMAP_H_
#define _TILEMAP_H_

class TileMap : public Tile
{
	unsigned int _mapWidth;
	unsigned int _mapHeight;

	TileSet* _tileSet;
	Factory<Tile> _tiles;
	TileLayerConfig _layerConfig;

public:
	explicit TileMap(
		unsigned int mapWidth,
		unsigned int mapHeight,
		TileSet* tileSet,
		const TileLayerConfig& layerConfig = TileLayerConfig()) :
		Tile(-1, tileSet),
		_mapWidth(mapWidth),
		_mapHeight(mapHeight),
		_tileSet(tileSet),
		_layerConfig(layerConfig) {

		if (_tileSet) {
			for (unsigned int i = 0; i < mapWidth; i++) {
				for (unsigned int j = 0; j < mapHeight; j++) {
					Tile* tile = _tiles.create();
					tile->setTileSet(_tileSet);
					tile->setLayerCollisionMode(_layerConfig.collisionMode);
				}
			}
		}
	}

	virtual ~TileMap(void) {
		_tiles.clear();
	}

	void setTileIndex(unsigned int x, unsigned int y, int tileIndex) {
		if (Tile* tile = this->getTile(x, y)) {
			tile->setTileIndex(tileIndex);
			tile->setLayerCollisionMode(_layerConfig.collisionMode);
		}
	}

	int getTileIndex(unsigned int x, unsigned int y) {
		if (Tile* tile = this->getTile(x, y)) {
			return tile->getTileIndex();
		}
		return -1;
	}

	unsigned int getMapWidth(void) const {
		return _mapWidth;
	}

	unsigned int getMapHeight(void) const {
		return _mapHeight;
	}

	Factory<Tile>& getTiles(void) { return _tiles; }
	const TileLayerConfig& getLayerConfig(void) const { return _layerConfig; }

	Tile* getTile(unsigned int x, unsigned int y) {
		if (_tileSet && x < _mapWidth && y < _mapHeight) {
			const unsigned int index = x + y * _mapWidth;
			if (index < _tiles.size()) {
				return _tiles[index];
			}
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
					const int layerTileX = (int)x + _layerConfig.startX;
					const int layerTileY = (int)y + _layerConfig.startY;
					tile->setPosition(this->getPosition() + vector2{
						_layerConfig.offsetX + (float)(layerTileX * (int)tileSize),
						_layerConfig.offsetY + (float)(layerTileY * (int)tileSize)
					});
					tile->setLayerCollisionMode(_layerConfig.collisionMode);
					tile->update(0);
#if _DEBUG
					if (Debug::dbgTiles) {
						if (Collidable* collidable = tile->getCollidable()) {
							char buffer[128];
							sprintf_s(buffer, sizeof(buffer), "pos{%f, %f}\tcpos{%f, %f}\n", tile->_x, tile->_y, collidable->_position.x, collidable->_position.y);
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
			} while (file.good());

			unsigned int mapHeight = (unsigned int)lines.size();
			unsigned int mapWidth = (unsigned int)lines[0].size();

			if (tileSet) {
				TileLayerConfig config;
				config.name = "csv";
				config.collisionMode = TileCollisionMode::Solid;
				tileMap = new TileMap(mapWidth, mapHeight, tileSet, config);

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

	static std::vector<TileMap*> loadFromJSONFile(const char* filePath, TileSet* tileSet)
	{
		if (!FileSystem::FileExists(filePath))
			return {};

		std::vector<TileMap*> tileMaps;
		simdjson::dom::parser parser;
		simdjson::dom::element json = parser.load(filePath);

		if (!json.is_object()) {
			return tileMaps;
		}

		auto readFloat = [](simdjson::dom::element element, float fallback = 0.0f) -> float {
			if (element.is_null()) {
				return fallback;
			}
			if (element.is_double()) {
				return (float)element.get_double();
			}
			if (element.is_int64()) {
				return (float)element.get_int64();
			}
			if (element.is_uint64()) {
				return (float)element.get_uint64();
			}
			return fallback;
		};

		auto readInt = [&](simdjson::dom::element element, int fallback = 0) -> int {
			return (int)std::round(readFloat(element, (float)fallback));
		};

		auto normalizeModeString = [](const std::string& value) -> std::string {
			std::string normalized = value;
			std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
				return (char)std::tolower(c);
			});
			return normalized;
		};

		auto parseCollisionMode = [&](const std::string& value, TileCollisionMode fallback) -> TileCollisionMode {
			const std::string normalized = normalizeModeString(value);
			if (normalized == "solid") {
				return TileCollisionMode::Solid;
			}
			if (normalized == "one_way" || normalized == "oneway") {
				return TileCollisionMode::OneWay;
			}
			if (normalized == "none") {
				return TileCollisionMode::None;
			}
			return fallback;
		};

		const int rootWidth = readInt(json["width"], 0);
		const int rootHeight = readInt(json["height"], 0);

		for (auto layer : json["layers"]) {
			const char* type = nullptr;
			if (layer["type"].is_string()) {
				type = layer["type"].get_c_str();
			}

			if (!type) {
				continue;
			}

			if (strcmp(type, "tilelayer") == 0) {
				const int mapWidth = readInt(layer["width"], rootWidth);
				const int mapHeight = readInt(layer["height"], rootHeight);
				if (mapWidth <= 0 || mapHeight <= 0) {
					continue;
				}

				TileLayerConfig layerConfig;
				layerConfig.id = readInt(layer["id"], -1);
				layerConfig.name = layer["name"].is_string() ? std::string((std::string_view)layer["name"].get_string()) : "";
				layerConfig.startX = readInt(layer["startx"], 0);
				layerConfig.startY = readInt(layer["starty"], 0);
				layerConfig.offsetX = readFloat(layer["x"], 0.0f);
				layerConfig.offsetY = readFloat(layer["y"], 0.0f);
				layerConfig.drawOrder = (int)tileMaps.size();

				// User-selected default for missing property is non-colliding.
				layerConfig.collisionMode = TileCollisionMode::None;

				if (!layer["properties"].is_null() && layer["properties"].is_array()) {
					for (auto property : layer["properties"]) {
						if (!property["name"].is_string()) {
							continue;
						}

						std::string propertyName = std::string((std::string_view)property["name"].get_string());
						std::transform(propertyName.begin(), propertyName.end(), propertyName.begin(), [](unsigned char c) {
							return (char)std::tolower(c);
						});
						if (propertyName != "collision_mode") {
							continue;
						}

						if (!property["value"].is_string()) {
#if _DEBUG
							char buffer[256];
							sprintf_s(buffer, sizeof(buffer),
								"Tile layer '%s' has non-string collision_mode; defaulting to 'none'\n",
								layerConfig.name.c_str());
							DEBUG_MSG(buffer);
#endif
							continue;
						}

						const std::string rawMode = std::string((std::string_view)property["value"].get_string());
						const TileCollisionMode parsedMode = parseCollisionMode(rawMode, TileCollisionMode::None);
						if (parsedMode == TileCollisionMode::None && normalizeModeString(rawMode) != "none") {
#if _DEBUG
							char buffer[256];
							sprintf_s(buffer, sizeof(buffer),
								"Unknown collision_mode '%s' on layer '%s'; defaulting to 'none'\n",
								rawMode.c_str(),
								layerConfig.name.c_str());
							DEBUG_MSG(buffer);
#endif
						}
						layerConfig.collisionMode = parsedMode;
					}
				}

				TileMap* tileMap = new TileMap((unsigned int)mapWidth, (unsigned int)mapHeight, tileSet, layerConfig);

				if (!layer["chunks"].is_null() && layer["chunks"].is_array()) {
					for (auto chunk : layer["chunks"]) {
						auto data = chunk["data"].get_array();
						const int chunkW = readInt(chunk["width"], 0);
						const int chunkH = readInt(chunk["height"], 0);
						const int chunkXoff = readInt(chunk["x"], 0);
						const int chunkYoff = readInt(chunk["y"], 0);
						int index = 0;

						for (int cy = 0; cy < chunkH; ++cy) {
							for (int cx = 0; cx < chunkW; ++cx, ++index) {
								if (index >= (int)data.size()) {
									break;
								}

								int64_t gid = data.at(index).get_int64();
								if (gid == 0) {
									continue;
								}

								const int layerX = cx + chunkXoff;
								const int layerY = cy + chunkYoff;
								const int localX = layerX - layerConfig.startX;
								const int localY = layerY - layerConfig.startY;
								if (localX < 0 || localY < 0 || localX >= mapWidth || localY >= mapHeight) {
									continue;
								}

								const int tileIndex = (int)(gid - 1);
								tileMap->setTileIndex((unsigned int)localX, (unsigned int)localY, tileIndex);
							}
						}
					}
				}
				else if (!layer["data"].is_null() && layer["data"].is_array()) {
					auto data = layer["data"].get_array();
					for (int localY = 0; localY < mapHeight; ++localY) {
						for (int localX = 0; localX < mapWidth; ++localX) {
							const int index = localX + localY * mapWidth;
							if (index >= (int)data.size()) {
								continue;
							}

							int64_t gid = data.at(index).get_int64();
							if (gid == 0) {
								continue;
							}

							tileMap->setTileIndex((unsigned int)localX, (unsigned int)localY, (int)(gid - 1));
						}
					}
				}

				tileMap->arrangeTiles();
				tileMaps.push_back(tileMap);
			}
			else if (strcmp(type, "objectgroup") == 0) {
				// TODO: parse object layers in a dedicated pass.
				continue;
			}
			else if (strcmp(type, "imagelayer") == 0) {
				continue;
			}
		}

		return tileMaps;
	}
};

#endif
