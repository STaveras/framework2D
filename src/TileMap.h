// TileMap.h
#pragma once

// TODO: Create a collidable encompassing the map

#include "Tile.h"
#include "StrUtil.h"
#include "FileSystem.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <memory>
#include <vector>

#ifndef _TILEMAP_H_
#define _TILEMAP_H_

class TileMap : public Tile
{
	struct OwnedTileSetRegistry {
		std::vector<TileSet*> tileSets;
		~OwnedTileSetRegistry() {
			for (TileSet* tileSet : tileSets) {
				delete tileSet;
			}
			tileSets.clear();
		}
	};

	unsigned int _mapWidth;
	unsigned int _mapHeight;

	TileSet* _tileSet;
	Factory<Tile> _tiles;
	std::vector<Tile*> _tileGrid;
	TileLayerConfig _layerConfig;
	std::shared_ptr<OwnedTileSetRegistry> _ownedTileSets;

	size_t tileGridIndex(unsigned int x, unsigned int y) const {
		return (size_t)x + ((size_t)y * (size_t)_mapWidth);
	}

	Tile* ensureTile(unsigned int x, unsigned int y) {
		if (x >= _mapWidth || y >= _mapHeight) {
			return nullptr;
		}

		const size_t index = tileGridIndex(x, y);
		Tile* tile = _tileGrid[index];
		if (!tile) {
			tile = _tiles.create();
			tile->setLayerCollisionMode(_layerConfig.collisionMode);
			if (_tileSet) {
				tile->setTileSet(_tileSet);
			}
			_tileGrid[index] = tile;
		}
		return tile;
	}

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
		_tileGrid.resize((size_t)mapWidth * (size_t)mapHeight, nullptr);
	}

	virtual ~TileMap(void) {
		_tileGrid.clear();
		_tiles.clear();
	}

	void setTileIndex(unsigned int x, unsigned int y, int tileIndex) {
		if (tileIndex < 0) {
			if (Tile* tile = this->getTile(x, y)) {
				tile->setTileIndex(tileIndex);
			}
			return;
		}

		if (Tile* tile = this->ensureTile(x, y)) {
			tile->setTileIndex(tileIndex);
			tile->setLayerCollisionMode(_layerConfig.collisionMode);
		}
	}

	void setTile(unsigned int x, unsigned int y, TileSet* tileSet, int tileIndex) {
		if (tileIndex < 0 && !this->getTile(x, y)) {
			return;
		}

		if (Tile* tile = this->ensureTile(x, y)) {
			if (tileSet) {
				tile->setTileSet(tileSet);
			}
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
		if (x < _mapWidth && y < _mapHeight) {
			const size_t index = tileGridIndex(x, y);
			if (index < _tileGrid.size()) {
				return _tileGrid[index];
			}
		}

		return NULL;
	}

	void setMapWidth(unsigned int mapWidth) { _mapWidth = mapWidth; }
	void setMapHeight(unsigned int mapHeight) { _mapHeight = mapHeight; }
	void setOwnedTileSetRegistry(const std::shared_ptr<OwnedTileSetRegistry>& ownedTileSets) { _ownedTileSets = ownedTileSets; }

	void arrangeTiles(void) {
		for (unsigned int y = 0; y < _mapHeight; y++) {
			for (unsigned int x = 0; x < _mapWidth; x++) {
				if (Tile* tile = getTile(x, y)) {
					if (tile->getTileIndex() < 0 || !tile->getTileSet()) {
						continue;
					}

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

	static std::vector<TileMap*> loadFromJSONFile(const char* filePath)
	{
		return loadFromJSONFile(filePath, nullptr, true);
	}

	static std::vector<TileMap*> loadFromJSONFile(const char* filePath, TileSet* fallbackTileSet, bool arrangeLayerTiles = true)
	{
		if (!FileSystem::FileExists(filePath))
			return {};

		std::vector<TileMap*> tileMaps;
		simdjson::dom::parser parser;
		simdjson::dom::element json = parser.load(filePath);

		if (!json.is_object()) {
			return tileMaps;
		}

		auto readFloat = [](auto element, float fallback = 0.0f) -> float {
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

		auto readInt = [&](auto element, int fallback = 0) -> int {
			return (int)std::round(readFloat(element, (float)fallback));
		};

		auto readInt64 = [](auto element, int64_t fallback = 0) -> int64_t {
			if (element.is_null()) {
				return fallback;
			}
			if (element.is_int64()) {
				return (int64_t)element.get_int64();
			}
			if (element.is_uint64()) {
				return (int64_t)element.get_uint64();
			}
			if (element.is_double()) {
				return (int64_t)std::llround((double)element.get_double());
			}
			return fallback;
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
		const std::string mapDirectory = FileSystem::File::GetFilePath(filePath);

		struct MapTileSetEntry {
			int firstGid = 1;
			TileSet* tileSet = nullptr;
			std::string source;
		};

		std::vector<MapTileSetEntry> mapTileSets;
		std::shared_ptr<OwnedTileSetRegistry> ownedTileSets = std::make_shared<OwnedTileSetRegistry>();

		if (!json["tilesets"].is_null() && json["tilesets"].is_array()) {
			for (auto mapTileSet : json["tilesets"]) {
				const int firstGid = readInt(mapTileSet["firstgid"], -1);
				if (firstGid <= 0) {
					continue;
				}

				if (!mapTileSet["source"].is_string()) {
					continue;
				}

				std::string sourcePath = std::string((std::string_view)mapTileSet["source"].get_string());
				if (sourcePath.empty()) {
					continue;
				}

				std::string resolvedSourcePath = FileSystem::Path::ResolveFromBaseOrParent(sourcePath, mapDirectory);
				if (!FileSystem::FileExists(resolvedSourcePath)) {
#if _DEBUG
					char buffer[512];
					sprintf_s(
						buffer,
						sizeof(buffer),
						"TileMap::loadFromJSONFile missing tileset source '%s' (resolved='%s')\n",
						sourcePath.c_str(),
						resolvedSourcePath.c_str());
					DEBUG_MSG(buffer);
#endif
					continue;
				}

				TileSet* loadedTileSet = TileSet::loadFromFile(resolvedSourcePath.c_str());
				if (!loadedTileSet) {
					continue;
				}

				mapTileSets.push_back({ firstGid, loadedTileSet, sourcePath });
				ownedTileSets->tileSets.push_back(loadedTileSet);
			}
		}

		std::sort(mapTileSets.begin(), mapTileSets.end(), [](const MapTileSetEntry& lhs, const MapTileSetEntry& rhs) {
			return lhs.firstGid < rhs.firstGid;
		});

		auto resolveTileFromGid = [&](int64_t gid, TileSet*& outTileSet, int& outTileIndex) -> bool {
			outTileSet = nullptr;
			outTileIndex = -1;
			if (gid <= 0) {
				return false;
			}

			if (!mapTileSets.empty()) {
				for (int i = (int)mapTileSets.size() - 1; i >= 0; --i) {
					if (gid >= mapTileSets[i].firstGid) {
						outTileSet = mapTileSets[i].tileSet;
						outTileIndex = (int)(gid - mapTileSets[i].firstGid);
						return outTileSet != nullptr && outTileIndex >= 0;
					}
				}
				return false;
			}

			if (fallbackTileSet) {
				outTileSet = fallbackTileSet;
				outTileIndex = (int)(gid - 1);
				return outTileIndex >= 0;
			}

			return false;
		};

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

				TileSet* defaultTileSet = fallbackTileSet;
				if (!defaultTileSet && !mapTileSets.empty()) {
					defaultTileSet = mapTileSets.front().tileSet;
				}

				TileMap* tileMap = new TileMap((unsigned int)mapWidth, (unsigned int)mapHeight, defaultTileSet, layerConfig);
				if (!mapTileSets.empty()) {
					tileMap->setOwnedTileSetRegistry(ownedTileSets);
				}

				if (!layer["chunks"].is_null() && layer["chunks"].is_array()) {
					for (auto chunk : layer["chunks"]) {
						auto data = chunk["data"].get_array();
						const int chunkW = readInt(chunk["width"], 0);
						const int chunkH = readInt(chunk["height"], 0);
						const int chunkXoff = readInt(chunk["x"], 0);
						const int chunkYoff = readInt(chunk["y"], 0);
						if (chunkW <= 0 || chunkH <= 0) {
							continue;
						}

						int index = 0;
						const int maxChunkCells = chunkW * chunkH;
						for (auto gidElement : data) {
							if (index >= maxChunkCells) {
								break;
							}

							const int cx = index % chunkW;
							const int cy = index / chunkW;
							++index;

							int64_t gid = readInt64(gidElement, 0);
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

							TileSet* resolvedTileSet = nullptr;
							int resolvedTileIndex = -1;
							if (!resolveTileFromGid(gid, resolvedTileSet, resolvedTileIndex)) {
#if _DEBUG
								char buffer[256];
								sprintf_s(buffer, sizeof(buffer), "Skipping unresolved gid %lld on layer '%s'\n", (long long)gid, layerConfig.name.c_str());
								DEBUG_MSG(buffer);
#endif
								continue;
							}

							tileMap->setTile((unsigned int)localX, (unsigned int)localY, resolvedTileSet, resolvedTileIndex);
						}
					}
				}
				else if (!layer["data"].is_null() && layer["data"].is_array()) {
					auto data = layer["data"].get_array();
					const int maxLayerCells = mapWidth * mapHeight;
					int index = 0;
					for (auto gidElement : data) {
						if (index >= maxLayerCells) {
							break;
						}

						const int localX = index % mapWidth;
						const int localY = index / mapWidth;
						++index;

						int64_t gid = readInt64(gidElement, 0);
						if (gid == 0) {
							continue;
						}

						TileSet* resolvedTileSet = nullptr;
						int resolvedTileIndex = -1;
						if (!resolveTileFromGid(gid, resolvedTileSet, resolvedTileIndex)) {
#if _DEBUG
							char buffer[256];
							sprintf_s(buffer, sizeof(buffer), "Skipping unresolved gid %lld on layer '%s'\n", (long long)gid, layerConfig.name.c_str());
							DEBUG_MSG(buffer);
#endif
							continue;
						}

						tileMap->setTile((unsigned int)localX, (unsigned int)localY, resolvedTileSet, resolvedTileIndex);
					}
				}

				if (arrangeLayerTiles) {
					tileMap->arrangeTiles();
				}
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
