// Collidable.cpp

#include "Collidable.h"

#include "CollidableGroup.h"
#include "Debug.h"
#include "Factory.h"
#include "Polygon.h"
#include "PolygonDecomposition.h"
#include "Square.h"
#include "Scripting/JSONReader.h"

#include <string_view>
#include <vector>

using Scripting::JSON::readFloat;
using Scripting::JSON::readInt64;

Collidable* Collidable::loadFromObjectGroup(simdjson::dom::element objectGroup, Factory<Collidable>& owner,
	vector2 origin, const char* source, int* decomposedConcaveCount)
{
	if (!objectGroup["objects"].is_array()) {
		return NULL;
	}

	Collidable* collision = nullptr;
	CollidableGroup* collisionGroup = nullptr;

	for (auto object : objectGroup["objects"]) {
		const int64_t objectId = readInt64(object["id"], -1);
		const bool hasPolygon = !object["polygon"].is_null() && object["polygon"].is_array();
		const bool hasRectangle = !object["width"].is_null() && !object["height"].is_null();
		std::string_view collisionObjectType = object["type"].is_string() ? object["type"].get_string().value_unsafe() : "";

		Collidable* parsedCollision = nullptr;
		if (hasPolygon) {
			const float objectX = readFloat(object["x"]) + origin.x;
			const float objectY = readFloat(object["y"]) + origin.y;

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
					"Skipping invalid polygon collision object (%s, object=%lld, reason=%s)\n",
					source,
					(long long)objectId,
					PolygonDecomposition::resultCodeToString(decomposition.code));
				DEBUG_MSG(buffer);
#endif
			}
			else {
				if (decomposedConcaveCount && decomposition.wasConcave && decomposition.pieces.size() > 1) {
					++(*decomposedConcaveCount);
				}

				Collidable* polygonResult = nullptr;
				CollidableGroup* polygonGroup = nullptr;
				for (const std::vector<vector2>& piece : decomposition.pieces) {
					PolygonCollider* polygon = owner.createDerived<PolygonCollider>();
					polygon->setPosition(objectX, objectY);
					polygon->setLocalVertices(piece);

					if (!polygon->isValid()) {
#if _DEBUG
						char buffer[320];
						sprintf_s(
							buffer,
							sizeof(buffer),
							"Skipping invalid decomposed polygon piece (%s, object=%lld)\n",
							source,
							(long long)objectId);
						DEBUG_MSG(buffer);
#endif
						owner.destroy(polygon);
						continue;
					}

					if (!polygonResult) {
						polygonResult = polygon;
						continue;
					}

					if (!polygonGroup) {
						polygonGroup = owner.createDerived<CollidableGroup>();
						polygonGroup->push_back(polygonResult);
						polygonResult = polygonGroup;
					}
					polygonGroup->push_back(polygon);
				}

				parsedCollision = polygonResult;
			}
		}
		else if (collisionObjectType == "square" || hasRectangle) {
			Square* square = owner.createDerived<Square>();
			square->setPosition(readFloat(object["x"]) + origin.x, readFloat(object["y"]) + origin.y);
			square->setWidth(readFloat(object["width"]));
			square->setHeight(readFloat(object["height"]));
			parsedCollision = square;
		}

		if (!parsedCollision) {
			continue;
		}

		if (!collision) {
			collision = parsedCollision;
			continue;
		}

		if (!collisionGroup) {
			collisionGroup = owner.createDerived<CollidableGroup>();
			collisionGroup->push_back(collision);
			collision = collisionGroup;
		}

		collisionGroup->push_back(parsedCollision);
	}

	return collision;
}
