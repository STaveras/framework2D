// TileMap.h
#pragma once

// TODO: Create a collidable encompassing the map

#include "Tile.h"
#include "StrUtil.h"
#include "FileSystem.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#ifndef _TILEMAP_H_
#define _TILEMAP_H_

class TileMap;

struct TileMapPropertyDescriptor
{
	std::string name;
	std::string type;
	std::string value;
};

struct TileObjectDescriptor
{
	int layerId = -1;
	std::string layerName;
	int id = -1;
	std::string name;
	std::string typeName;
	int64_t gid = 0;
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
	float rotation = 0.0f;
	bool visible = true;
	bool isPoint = false;
	bool isEllipse = false;
	bool hasPolygon = false;
	bool hasPolyline = false;
	std::vector<vector2> polygonPoints;
	std::vector<vector2> polylinePoints;
	std::vector<TileMapPropertyDescriptor> properties;
	bool hasResolvedTileVisual = false;
	std::string tileTexturePath;
	RECT tileSrcRect{ 0, 0, 0, 0 };
	std::unordered_map<std::string, std::string> tileProperties;
	std::unordered_map<std::string, std::string> objectPropertyMap;
	std::unordered_map<std::string, std::string> mergedPropertyMap;

	const std::string* findProperty(const std::string& name) const {
		std::unordered_map<std::string, std::string>::const_iterator itr = mergedPropertyMap.find(name);
		if (itr == mergedPropertyMap.end()) {
			return NULL;
		}
		return &(itr->second);
	}

	bool hasObjectProperty(const std::string& name) const {
		return objectPropertyMap.find(name) != objectPropertyMap.end();
	}
};

struct TileObjectLayerDescriptor
{
	int id = -1;
	std::string name;
	bool visible = true;
	int traversalIndex = -1;
	std::vector<TileObjectDescriptor> objects;
};

struct TileImageLayerDescriptor
{
	int id = -1;
	std::string name;
	std::string className;
	bool visible = true;
	int traversalIndex = -1;
	std::string imagePath;
	float x = 0.0f;
	float y = 0.0f;
	float opacity = 1.0f;
	bool repeatX = false;
	bool repeatY = false;
	float parallaxX = 1.0f;
	float parallaxY = 1.0f;
};

struct MapLayerDescriptor
{
	int id = -1;
	std::string name;
	std::string type;
	std::string className;
	bool visible = true;
	int traversalIndex = -1;
	int typedIndex = -1;
};

struct TileMapLoadResult
{
	int mapWidth = 0;
	int mapHeight = 0;
	int tileWidth = 0;
	int tileHeight = 0;
	std::vector<MapLayerDescriptor> layers;
	std::vector<TileMap*> tileMaps;
	std::vector<TileObjectLayerDescriptor> objectLayers;
	std::vector<TileImageLayerDescriptor> imageLayers;
};

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
			tile->setLayerName(_layerConfig.name);
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

	void setTileIndex(unsigned int x, unsigned int y, int tileIndex) 
	{
		if (tileIndex < 0) {
			if (Tile* tile = this->getTile(x, y)) {
				tile->setTileIndex(tileIndex);
			}
			return;
		}

		if (Tile* tile = this->ensureTile(x, y)) {
			tile->setTileIndex(tileIndex);
			tile->setLayerCollisionMode(_layerConfig.collisionMode);
			tile->setLayerName(_layerConfig.name);
		}
	}

	void setTile(unsigned int x, unsigned int y, TileSet* tileSet, int tileIndex) 
	{
		if (tileIndex < 0 && !this->getTile(x, y)) {
			return;
		}

		if (Tile* tile = this->ensureTile(x, y)) {
			if (tileSet) {
				tile->setTileSet(tileSet);
			}
			tile->setTileIndex(tileIndex);
			tile->setLayerCollisionMode(_layerConfig.collisionMode);
			tile->setLayerName(_layerConfig.name);
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

	Tile* getTile(unsigned int x, unsigned int y) 
	{
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
					tile->setLayerName(_layerConfig.name);
					tile->update(0);
#if _DEBUG
					if (_layerConfig.collisionMode != TileCollisionMode::None && !tile->getCollidable()) {
						char buffer[256];
						sprintf_s(
							buffer,
							sizeof(buffer),
							"TileMap::arrangeTiles layer '%s' tileIndex=%d has collidable layer but no collision shape\n",
							_layerConfig.name.c_str(),
							tile->getTileIndex());
						DEBUG_MSG(buffer);
					}
#endif
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

static TileMap* loadFromCSVFile(const char* filePath, TileSet* tileSet)
{
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
		TileMapLoadResult loadResult = loadMapDataFromJSONFile(filePath, fallbackTileSet, arrangeLayerTiles);
		return std::move(loadResult.tileMaps);
	}

	static TileMapLoadResult loadMapDataFromJSONFile(const char* filePath)
	{
		return loadMapDataFromJSONFile(filePath, nullptr, true);
	}

	static TileMapLoadResult loadMapDataFromJSONFile(const char* filePath, TileSet* fallbackTileSet, bool arrangeLayerTiles = true)
	{
		TileMapLoadResult result;
		if (!FileSystem::FileExists(filePath))
			return result;

		simdjson::dom::parser parser;
		simdjson::dom::element json = parser.load(filePath);

		if (!json.is_object()) {
			return result;
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

		auto readBool = [](auto element, bool fallback = true) -> bool {
			if (element.is_null()) {
				return fallback;
			}
			if (element.is_bool()) {
				return (bool)element.get_bool();
			}
			if (element.is_int64()) {
				return element.get_int64() != 0;
			}
			if (element.is_uint64()) {
				return element.get_uint64() != 0;
			}
			return fallback;
		};

		auto readLayerOffset = [&](auto layerElement, const char* preferredFieldName, const char* fallbackFieldName) -> float {
			const auto preferred = layerElement[preferredFieldName];
			if (!preferred.is_null()) {
				return readFloat(preferred, 0.0f);
			}
			return readFloat(layerElement[fallbackFieldName], 0.0f);
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

			auto collisionModeToString = [](TileCollisionMode mode) -> const char* {
				switch (mode) {
				case TileCollisionMode::Solid:
					return "solid";
				case TileCollisionMode::OneWay:
					return "one_way";
				case TileCollisionMode::None:
				default:
					return "none";
				}
			};

		auto normalizeGid = [](int64_t rawGid) -> int64_t {
			const uint32_t rawValue = (uint32_t)(rawGid & 0xFFFFFFFFLL);
			return (int64_t)(rawValue & 0x1FFFFFFFu);
		};

		auto resolveClassOrType = [](auto classElement, auto typeElement) -> std::string {
			if (!classElement.is_null() && classElement.is_string()) {
				std::string classValue = std::string((std::string_view)classElement.get_string());
				if (!classValue.empty()) {
					return classValue;
				}
			}
			if (!typeElement.is_null() && typeElement.is_string()) {
				std::string typeValue = std::string((std::string_view)typeElement.get_string());
				if (!typeValue.empty()) {
					return typeValue;
				}
			}
			return "";
		};

		auto propertyValueToString = [](auto element) -> std::string {
			if (element.is_null()) {
				return "";
			}
			if (element.is_string()) {
				return std::string((std::string_view)element.get_string());
			}
			if (element.is_bool()) {
				return (bool)element.get_bool() ? "true" : "false";
			}
			if (element.is_int64()) {
				return std::to_string((int64_t)element.get_int64());
			}
			if (element.is_uint64()) {
				return std::to_string((uint64_t)element.get_uint64());
			}
			if (element.is_double()) {
				std::ostringstream stream;
				stream << (double)element.get_double();
				return stream.str();
			}
			return "";
		};

		result.mapWidth = readInt(json["width"], 0);
		result.mapHeight = readInt(json["height"], 0);
		result.tileWidth = readInt(json["tilewidth"], 0);
		result.tileHeight = readInt(json["tileheight"], 0);

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
						"TileMap::loadMapDataFromJSONFile missing tileset source '%s' (resolved='%s')\n",
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
			const int64_t normalizedGid = normalizeGid(gid);
			if (normalizedGid <= 0) {
				return false;
			}

			if (!mapTileSets.empty()) {
				for (int i = (int)mapTileSets.size() - 1; i >= 0; --i) {
					if (normalizedGid >= mapTileSets[i].firstGid) {
						outTileSet = mapTileSets[i].tileSet;
						outTileIndex = (int)(normalizedGid - mapTileSets[i].firstGid);
						return outTileSet != nullptr && outTileIndex >= 0;
					}
				}
				return false;
			}

			if (fallbackTileSet) {
				outTileSet = fallbackTileSet;
				outTileIndex = (int)(normalizedGid - 1);
				return outTileIndex >= 0;
			}

			return false;
		};

		if (!json["layers"].is_null() && json["layers"].is_array()) {
			int traversalIndex = 0;
			for (auto layer : json["layers"]) {
				MapLayerDescriptor layerDescriptor;
				layerDescriptor.id = readInt(layer["id"], -1);
				layerDescriptor.name = layer["name"].is_string() ? std::string((std::string_view)layer["name"].get_string()) : "";
				layerDescriptor.className = layer["class"].is_string() ? std::string((std::string_view)layer["class"].get_string()) : "";
				layerDescriptor.visible = readBool(layer["visible"], true);
				layerDescriptor.traversalIndex = traversalIndex++;

				const std::string layerType = layer["type"].is_string() ? std::string((std::string_view)layer["type"].get_string()) : "";
				layerDescriptor.type = layerType;

				if (layerType == "tilelayer") {
					const int mapWidth = readInt(layer["width"], result.mapWidth);
					const int mapHeight = readInt(layer["height"], result.mapHeight);
					if (mapWidth > 0 && mapHeight > 0) {
						TileLayerConfig layerConfig;
						layerConfig.id = layerDescriptor.id;
						layerConfig.name = layerDescriptor.name;
						layerConfig.startX = readInt(layer["startx"], 0);
						layerConfig.startY = readInt(layer["starty"], 0);
						// Tiled stores layer offsets in offsetx/offsety. Keep x/y as a fallback for legacy files.
						layerConfig.offsetX = readLayerOffset(layer, "offsetx", "x");
						layerConfig.offsetY = readLayerOffset(layer, "offsety", "y");
						layerConfig.drawOrder = layerDescriptor.traversalIndex;

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

							int nonEmptyTileCount = 0;

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

										const int64_t gid = normalizeGid(readInt64(gidElement, 0));
										if (gid == 0) {
											continue;
										}
										++nonEmptyTileCount;

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

									const int64_t gid = normalizeGid(readInt64(gidElement, 0));
									if (gid == 0) {
										continue;
									}
									++nonEmptyTileCount;

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

#if _DEBUG
							{
								char buffer[320];
								sprintf_s(
									buffer,
									sizeof(buffer),
									"TileMap::load layer '%s' mode=%s nonempty=%d visible=%s\\n",
									layerConfig.name.c_str(),
									collisionModeToString(layerConfig.collisionMode),
									nonEmptyTileCount,
									layerDescriptor.visible ? "true" : "false");
								DEBUG_MSG(buffer);
							}
#endif

							if (arrangeLayerTiles) {
								tileMap->arrangeTiles();
							}

						layerDescriptor.typedIndex = (int)result.tileMaps.size();
						result.tileMaps.push_back(tileMap);
					}
				}
				else if (layerType == "objectgroup") {
					TileObjectLayerDescriptor objectLayer;
					objectLayer.id = layerDescriptor.id;
					objectLayer.name = layerDescriptor.name;
					objectLayer.visible = layerDescriptor.visible;
					objectLayer.traversalIndex = layerDescriptor.traversalIndex;

					const float layerOffsetX = readLayerOffset(layer, "offsetx", "x");
					const float layerOffsetY = readLayerOffset(layer, "offsety", "y");
					if (!layer["objects"].is_null() && layer["objects"].is_array()) {
						for (auto object : layer["objects"]) {
							TileObjectDescriptor descriptor;
							descriptor.layerId = objectLayer.id;
							descriptor.layerName = objectLayer.name;
							descriptor.id = readInt(object["id"], -1);
							descriptor.name = object["name"].is_string() ? std::string((std::string_view)object["name"].get_string()) : "";
							descriptor.gid = normalizeGid(readInt64(object["gid"], 0));
							descriptor.x = layerOffsetX + readFloat(object["x"], 0.0f);
							descriptor.y = layerOffsetY + readFloat(object["y"], 0.0f);
							descriptor.width = readFloat(object["width"], 0.0f);
							descriptor.height = readFloat(object["height"], 0.0f);
							descriptor.rotation = readFloat(object["rotation"], 0.0f);
							descriptor.visible = readBool(object["visible"], true);
							descriptor.isPoint = readBool(object["point"], false);
							descriptor.isEllipse = readBool(object["ellipse"], false);
							descriptor.typeName = resolveClassOrType(object["class"], object["type"]);

							TileSet* resolvedTileSet = nullptr;
							int resolvedTileIndex = -1;
							if (descriptor.gid > 0 &&
								resolveTileFromGid(descriptor.gid, resolvedTileSet, resolvedTileIndex) &&
								resolvedTileSet &&
								resolvedTileIndex >= 0) {
								const TileSet::TileInfo resolvedTileInfo = resolvedTileSet->getTileInfo(resolvedTileIndex);
								if (descriptor.typeName.empty()) {
									descriptor.typeName = resolvedTileInfo._typeName;
								}

								descriptor.tileProperties = resolvedTileInfo._properties;
								if (Texture* tileSheet = resolvedTileSet->getTileSheet()) {
									const char* filename = tileSheet->getFilename();
									if (filename && filename[0] != '\0') {
										descriptor.tileTexturePath = filename;
										const int tileSize = (int)resolvedTileSet->getTileSize();
										int columns = (int)resolvedTileSet->getTileCounts().x;
										if (columns <= 0) {
											columns = 1;
										}
										const int srcX = (resolvedTileIndex % columns) * tileSize;
										const int srcY = (resolvedTileIndex / columns) * tileSize;
										descriptor.tileSrcRect = RECT{ srcX, srcY, srcX + tileSize, srcY + tileSize };
										descriptor.hasResolvedTileVisual = true;
									}
								}
							}

							if (!object["polygon"].is_null() && object["polygon"].is_array()) {
								descriptor.hasPolygon = true;
								for (auto point : object["polygon"]) {
									descriptor.polygonPoints.push_back(vector2(
										readFloat(point["x"], 0.0f),
										readFloat(point["y"], 0.0f)));
								}
							}

							if (!object["polyline"].is_null() && object["polyline"].is_array()) {
								descriptor.hasPolyline = true;
								for (auto point : object["polyline"]) {
									descriptor.polylinePoints.push_back(vector2(
										readFloat(point["x"], 0.0f),
										readFloat(point["y"], 0.0f)));
								}
							}

							if (!object["properties"].is_null() && object["properties"].is_array()) {
								for (auto property : object["properties"]) {
									if (!property["name"].is_string()) {
										continue;
									}

									TileMapPropertyDescriptor propertyDescriptor;
									propertyDescriptor.name = std::string((std::string_view)property["name"].get_string());
									propertyDescriptor.type = property["type"].is_string() ? std::string((std::string_view)property["type"].get_string()) : "";
									propertyDescriptor.value = propertyValueToString(property["value"]);
									descriptor.properties.push_back(propertyDescriptor);
									descriptor.objectPropertyMap[propertyDescriptor.name] = propertyDescriptor.value;
								}
							}

							descriptor.mergedPropertyMap = descriptor.tileProperties;
							for (const auto& objectProperty : descriptor.objectPropertyMap) {
								descriptor.mergedPropertyMap[objectProperty.first] = objectProperty.second;
							}

							objectLayer.objects.push_back(descriptor);
						}
					}

					layerDescriptor.typedIndex = (int)result.objectLayers.size();
					result.objectLayers.push_back(std::move(objectLayer));
				}
				else if (layerType == "imagelayer") {
					TileImageLayerDescriptor imageLayer;
					imageLayer.id = layerDescriptor.id;
					imageLayer.name = layerDescriptor.name;
					imageLayer.className = layerDescriptor.className;
					imageLayer.visible = layerDescriptor.visible;
					imageLayer.traversalIndex = layerDescriptor.traversalIndex;
					imageLayer.x = readLayerOffset(layer, "offsetx", "x");
					imageLayer.y = readLayerOffset(layer, "offsety", "y");
					imageLayer.opacity = readFloat(layer["opacity"], 1.0f);
					imageLayer.repeatX = readBool(layer["repeatx"], false);
					imageLayer.repeatY = readBool(layer["repeaty"], false);
					imageLayer.parallaxX = readFloat(layer["parallaxx"], 1.0f);
					imageLayer.parallaxY = readFloat(layer["parallaxy"], 1.0f);

					if (layer["image"].is_string()) {
						const std::string imagePath = std::string((std::string_view)layer["image"].get_string());
						imageLayer.imagePath = FileSystem::Path::ResolveFromBaseOrParent(imagePath, mapDirectory);
					}

					layerDescriptor.typedIndex = (int)result.imageLayers.size();
					result.imageLayers.push_back(std::move(imageLayer));
				}

				result.layers.push_back(std::move(layerDescriptor));
			}
		}

		return result;
	}
};

#endif
