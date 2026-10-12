// TileSet.cpp

#include "TileSet.h"

#include "CollidableGroup.h"
#include "Engine2D.h"
#include "FileSystem.h"
#include "Polygon.h"
#include "Square.h"
#include "StrUtils.h"
#include "Scripting/JSONReader.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

// I usually hate globals, but this one will only be accessible to TileSets
// Eventually this might grow too large if we're loading many tilesets and not clearing this
static Factory<Collidable> collisionObjects;
static TileSet::CollisionLoadStats gCollisionLoadStats;

TileSet* TileSet::loadFromFile(const char* fileName)
{
	TileSet* tileSet = NULL;

	simdjson::dom::parser parser;
	simdjson::dom::element root = parser.load(fileName);

	if (!root.is_null()) {
		std::string workingDirectory = FileSystem::File::GetFilePath(fileName);
		std::string_view imageName = "";
		if (root["image"].is_string()) {
			imageName = root["image"].get_string();
		}

		const bool hasTileWidth = !root["tilewidth"].is_null() && (root["tilewidth"].is_int64() || root["tilewidth"].is_uint64());
		const bool hasTileHeight = !root["tileheight"].is_null() && (root["tileheight"].is_int64() || root["tileheight"].is_uint64());
		const bool hasTileCount = !root["tilecount"].is_null() && (root["tilecount"].is_int64() || root["tilecount"].is_uint64());

		if (!imageName.empty() && hasTileWidth && hasTileHeight && hasTileCount) {
			const int64_t tileWidth = root["tilewidth"].get_int64();
			const int64_t tileHeight = root["tileheight"].get_int64();
			const int64_t tileCount = root["tilecount"].get_int64();
			(void)tileHeight;

			std::string imagePath = workingDirectory + "/";
			imagePath.append(imageName);

			tileSet = new TileSet(Engine2D::getRenderer()->createTexture(imagePath.c_str()), (unsigned int)tileWidth);

			auto resolveClassOrType = [](simdjson::dom::element tileElement) -> std::string {
				if (!tileElement["class"].is_null() && tileElement["class"].is_string()) {
					std::string_view className = tileElement["class"].get_string();
					if (!className.empty()) {
						return std::string(className);
					}
				}
				if (!tileElement["type"].is_null() && tileElement["type"].is_string()) {
					std::string_view typeName = tileElement["type"].get_string();
					if (!typeName.empty()) {
						return std::string(typeName);
					}
				}
				return "";
			};

			int explicitColliderCount = 0;
			int missingColliderCount = 0;
			int decomposedConcaveCount = 0;

			if (!root["tiles"].is_null() && root["tiles"].is_array()) {
				simdjson::dom::array tiles = root["tiles"].get_array();

				for (simdjson::dom::element tile : tiles) {
					const bool hasIntId = tile["id"].is_int64();
					const bool hasUIntId = tile["id"].is_uint64();
					if (!hasIntId && !hasUIntId) {
						continue;
					}

					const int64_t id = hasIntId ? (int64_t)tile["id"].get_int64() : (int64_t)tile["id"].get_uint64();
					if (id < 0 || id >= tileCount) {
						continue;
					}

					TileSet::TileInfo tileInfo;
					tileInfo._typeName = resolveClassOrType(tile);

					if (!tile["properties"].is_null() && tile["properties"].is_array()) {
						for (simdjson::dom::element property : tile["properties"]) {
							if (!property["name"].is_string()) {
								continue;
							}
							const std::string name((std::string_view)property["name"].get_string());
							tileInfo._properties[name] = Scripting::JSON::readText(property["value"]);
						}
					}

					if (!tile["objectgroup"].is_null() && tile["objectgroup"].is_object()) {
						const std::string source = "tile=" + std::to_string((long long)id);
						tileInfo._collisionInfo = Collidable::loadFromObjectGroup(tile["objectgroup"], collisionObjects,
							vector2(0.0f, 0.0f), source.c_str(), &decomposedConcaveCount);
					}

					if (!tileInfo._typeName.empty() || tileInfo._collisionInfo != NULL || !tileInfo._properties.empty()) {
						tileSet->_tileInfo[(int)id] = tileInfo;
					}
				}

				tileSet->resolveUVLoops();
			}

			for (int64_t i = 0; i < tileCount; ++i) {
				auto tileInfoItr = tileSet->_tileInfo.find((int)i);
				if (tileInfoItr == tileSet->_tileInfo.end() || !tileInfoItr->second._collisionInfo) {
					++missingColliderCount;
				}
				else {
					++explicitColliderCount;
				}
			}

			gCollisionLoadStats.explicitColliders += explicitColliderCount;
			gCollisionLoadStats.missingColliders += missingColliderCount;
			gCollisionLoadStats.decomposedConcavePolygons += decomposedConcaveCount;
		}
#if _DEBUG
		else {
			char buffer[512];
			sprintf_s(
				buffer,
				sizeof(buffer),
				"TileSet::loadFromFile invalid or missing required field(s) in '%s' (image/tilewidth/tileheight/tilecount)\n",
				fileName ? fileName : "(null)");
			DEBUG_MSG(buffer);
		}
#endif
	}

	return tileSet;
}

void TileSet::resolveUVLoops(void)
{
	const int columns = (int)_tileCounts.x;
	const int rows = (int)_tileCounts.y;
	if (columns <= 0 || rows <= 0) {
		return;
	}

	auto loopName = [this, columns](int column, int row, const char* property) -> std::string {
		const TileInfo* info = findTileInfo(row * columns + column);
		if (!info) {
			return std::string();
		}
		auto itr = info->_properties.find(property);
		return (itr != info->_properties.end()) ? itr->second : std::string();
	};

	const int size = (int)_tileSize;
	for (auto& entry : _tileInfo) {
		const int column = entry.first % columns;
		const int row = entry.first / columns;
		UVLoop& loop = entry.second._uvLoop;

		const std::string nameX = loopName(column, row, "uv_loop_x");
		if (!nameX.empty()) {
			int first = column;
			while (first > 0 && loopName(first - 1, row, "uv_loop_x") == nameX) {
				--first;
			}
			int last = column;
			while (last + 1 < columns && loopName(last + 1, row, "uv_loop_x") == nameX) {
				++last;
			}
			loop.left = first * size;
			loop.width = (last - first + 1) * size;
		}

		const std::string nameY = loopName(column, row, "uv_loop_y");
		if (!nameY.empty()) {
			int first = row;
			while (first > 0 && loopName(column, first - 1, "uv_loop_y") == nameY) {
				--first;
			}
			int last = row;
			while (last + 1 < rows && loopName(column, last + 1, "uv_loop_y") == nameY) {
				++last;
			}
			loop.top = first * size;
			loop.height = (last - first + 1) * size;
		}
	}
}

TileSet::CollisionLoadStats TileSet::getCollisionLoadStats(void)
{
	return gCollisionLoadStats;
}

void TileSet::resetCollisionLoadStats(void)
{
	gCollisionLoadStats = TileSet::CollisionLoadStats();
}

vector2 TileSet::flipTilePoint(vector2 point, float size, unsigned int flipFlags)
{
	if (flipFlags & kFlipDiagonal) {
		point = vector2(point.y, point.x);
	}
	if (flipFlags & kFlipHorizontal) {
		point.x = size - point.x;
	}
	if (flipFlags & kFlipVertical) {
		point.y = size - point.y;
	}
	return point;
}

// Builds a copy of a tile-local collider with the flip flags applied.
static Collidable* cloneFlippedCollision(const Collidable* source, float size, unsigned int flipFlags)
{
	if (!source) {
		return NULL;
	}

	switch (source->getType()) {
	case COL_OBJ_SQUARE: {
		const Square* square = (const Square*)source;
		const vector2 a = TileSet::flipTilePoint(square->getPosition(), size, flipFlags);
		const vector2 b = TileSet::flipTilePoint(
			square->getPosition() + vector2(square->getWidth(), square->getHeight()), size, flipFlags);
		Square* flipped = collisionObjects.createDerived<Square>();
		flipped->setPosition(std::min(a.x, b.x), std::min(a.y, b.y));
		flipped->setWidth(std::abs(b.x - a.x));
		flipped->setHeight(std::abs(b.y - a.y));
		return flipped;
	}
	case COL_OBJ_POLYGON: {
		const PolygonCollider* polygon = (const PolygonCollider*)source;
		std::vector<vector2> vertices;
		vertices.reserve(polygon->getLocalVertices().size());
		for (const vector2& vertex : polygon->getLocalVertices()) {
			vertices.push_back(TileSet::flipTilePoint(polygon->getPosition() + vertex, size, flipFlags));
		}

		// Each flip is a reflection; an odd number of them reverses the winding, so
		// restore the original order to keep edge normals facing the same way.
		const bool h = (flipFlags & TileSet::kFlipHorizontal) != 0;
		const bool v = (flipFlags & TileSet::kFlipVertical) != 0;
		const bool d = (flipFlags & TileSet::kFlipDiagonal) != 0;
		if (h ^ v ^ d) {
			std::reverse(vertices.begin(), vertices.end());
		}

		PolygonCollider* flipped = collisionObjects.createDerived<PolygonCollider>();
		flipped->setPosition(0.0f, 0.0f);
		flipped->setLocalVertices(vertices);
		return flipped;
	}
	case COL_OBJ_GROUP: {
		CollidableGroup* flipped = collisionObjects.createDerived<CollidableGroup>();
		for (const Collidable* member : *(const CollidableGroup*)source) {
			if (Collidable* flippedMember = cloneFlippedCollision(member, size, flipFlags)) {
				flipped->push_back(flippedMember);
			}
		}
		return flipped;
	}
	default:
		// Tile colliders are only loaded as squares and polygons.
		return (Collidable*)source;
	}
}

Collidable* TileSet::getCollision(int tileIndex, unsigned int flipFlags)
{
	auto infoItr = _tileInfo.find(tileIndex);
	if (infoItr == _tileInfo.end() || !infoItr->second._collisionInfo) {
		return NULL;
	}

	flipFlags &= kFlipMask;
	if (flipFlags == 0) {
		return infoItr->second._collisionInfo;
	}

	const std::pair<int, unsigned int> key(tileIndex, flipFlags);
	auto flippedItr = _flippedCollision.find(key);
	if (flippedItr != _flippedCollision.end()) {
		return flippedItr->second;
	}

	Collidable* flipped = cloneFlippedCollision(infoItr->second._collisionInfo, getTileSize(), flipFlags);
	_flippedCollision[key] = flipped;
	return flipped;
}

float TileSet::TileInfo::getFloatProperty(const std::string& name, float fallback) const
{
	auto itr = _properties.find(name);
	return (itr == _properties.end()) ? fallback : StrUtils::floatFromString(itr->second, fallback);
}

int TileSet::TileInfo::getIntProperty(const std::string& name, int fallback) const
{
	auto itr = _properties.find(name);
	return (itr == _properties.end()) ? fallback : StrUtils::intFromString(itr->second, fallback);
}
