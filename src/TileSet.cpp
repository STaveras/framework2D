// TileSet.cpp

#include "TileSet.h"

#include "CollidableGroup.h"
#include "Engine2D.h"
#include "FileSystem.h"
#include "Polygon.h"
#include "PolygonDecomposition.h"
#include "Square.h"

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

			auto readInt64 = [](simdjson::dom::element element, int64_t fallback = -1) -> int64_t {
				if (element.is_null()) {
					return fallback;
				}
				if (element.is_int64()) {
					return (int64_t)element.get_int64();
				}
				if (element.is_uint64()) {
					return (int64_t)element.get_uint64();
				}
				return fallback;
			};

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

					if (!tile["objectgroup"].is_null() && tile["objectgroup"].is_object()) {
						simdjson::dom::element objectgroup = tile["objectgroup"];
						if (!objectgroup["objects"].is_null() && objectgroup["objects"].is_array()) {
							Collidable* tileCollision = nullptr;
							CollidableGroup* tileCollisionGroup = nullptr;

							for (auto object : objectgroup["objects"]) {
								const int64_t objectId = readInt64(object["id"], -1);
								const bool hasPolygon = !object["polygon"].is_null() && object["polygon"].is_array();
								const bool hasRectangle = !object["width"].is_null() && !object["height"].is_null();
								std::string_view collisionObjectType = object["type"].is_string() ? object["type"].get_string().value_unsafe() : "";

								Collidable* parsedCollision = nullptr;
								if (hasPolygon) {
									const float objectX = readFloat(object["x"]);
									const float objectY = readFloat(object["y"]);

									std::vector<vector2> polygonVertices;
									for (auto point : object["polygon"]) {
										polygonVertices.push_back(vector2(
											readFloat(point["x"]),
											readFloat(point["y"])));
									}

									PolygonDecomposition::Result decomposition = PolygonDecomposition::decomposeToConvex(polygonVertices);
									if (decomposition.code != PolygonDecomposition::ResultCode::Success || decomposition.pieces.empty()) {
#if _DEBUG
										char buffer[384];
										sprintf_s(
											buffer,
											sizeof(buffer),
											"Skipping invalid polygon collision object (tile=%lld, object=%lld, reason=%s)\n",
											(long long)id,
											(long long)objectId,
											PolygonDecomposition::resultCodeToString(decomposition.code));
										DEBUG_MSG(buffer);
#endif
									}
									else {
										if (decomposition.wasConcave && decomposition.pieces.size() > 1) {
											++decomposedConcaveCount;
										}

										Collidable* polygonResult = nullptr;
										CollidableGroup* polygonGroup = nullptr;
										for (const std::vector<vector2>& piece : decomposition.pieces) {
											PolygonCollider* polygon = collisionObjects.createDerived<PolygonCollider>();
											polygon->setPosition(objectX, objectY);
											polygon->setLocalVertices(piece);

											if (!polygon->isValid()) {
#if _DEBUG
												char buffer[320];
												sprintf_s(
													buffer,
													sizeof(buffer),
													"Skipping invalid decomposed polygon piece (tile=%lld, object=%lld)\n",
													(long long)id,
													(long long)objectId);
												DEBUG_MSG(buffer);
#endif
												collisionObjects.destroy(polygon);
												continue;
											}

											if (!polygonResult) {
												polygonResult = polygon;
												continue;
											}

											if (!polygonGroup) {
												polygonGroup = collisionObjects.createDerived<CollidableGroup>();
												polygonGroup->push_back(polygonResult);
												polygonResult = polygonGroup;
											}
											polygonGroup->push_back(polygon);
										}

										parsedCollision = polygonResult;
									}
								}
								else if (collisionObjectType == "square" || hasRectangle) {
									Square* square = collisionObjects.createDerived<Square>();
									square->setPosition(readFloat(object["x"]), readFloat(object["y"]));
									square->setWidth(readFloat(object["width"]));
									square->setHeight(readFloat(object["height"]));
									parsedCollision = square;
								}

								if (!parsedCollision) {
									continue;
								}

								if (!tileCollision) {
									tileCollision = parsedCollision;
									continue;
								}

								if (!tileCollisionGroup) {
									tileCollisionGroup = collisionObjects.createDerived<CollidableGroup>();
									tileCollisionGroup->push_back(tileCollision);
									tileCollision = tileCollisionGroup;
								}

								tileCollisionGroup->push_back(parsedCollision);
							}

							tileInfo._collisionInfo = tileCollision;
						}
					}

					if (!tileInfo._typeName.empty() || tileInfo._collisionInfo != NULL) {
						tileSet->_tileInfo[(int)id] = tileInfo;
					}
				}
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

TileSet::CollisionLoadStats TileSet::getCollisionLoadStats(void)
{
	return gCollisionLoadStats;
}

void TileSet::resetCollisionLoadStats(void)
{
	gCollisionLoadStats = TileSet::CollisionLoadStats();
}
