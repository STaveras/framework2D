// LevelProps.cpp
#include "LevelProps.h"

#include "Character.h"

#include "../Debug.h"
#include "../Kinematics2D.h"
#include "../TileMap.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <cstring>
#include <limits>

namespace
{
constexpr float kDefaultRiseSpeedScale = 2.0f;
constexpr float kDefaultSinkDepth = 24.0f;
// How close the character's feet must be to a platform's top to count as standing on it.
constexpr float kStandTolerance = 3.0f;
// Reach around a chest or door within which the interact action opens it.
constexpr float kChestReach = 6.0f;
// Keys rise this many pixels above their painted cell and back, one cycle every two seconds.
constexpr float kKeyBobHeight = 2.0f;
constexpr float kKeyBobSpeed = 3.14159f;

const TileSet::TileInfo* infoOf(const Tile* tile)
{
	if (!tile || tile->getTileIndex() < 0 || !tile->getTileSet()) {
		return nullptr;
	}
	return tile->getTileSet()->findTileInfo(tile->getTileIndex());
}

bool isClass(const Tile* tile, const char* className)
{
	const TileSet::TileInfo* info = infoOf(tile);
	return info && info->_typeName == className;
}

bool sinks(const Tile* tile)
{
	const TileSet::TileInfo* info = infoOf(tile);
	return info && info->getFloatProperty("sink_speed", 0.0f) > 0.0f;
}

// Whether neighbour lies at (dx, dy) from tile in the map as their part coordinates say it
// should, once the coordinates are transformed just as Tiled transforms the art.
bool partsLineUp(const Tile* tile, vector2 part, const Tile* neighbour, vector2 neighbourPart, int dx, int dy)
{
	const vector2 offset = TileSet::flipTilePoint(neighbourPart, 1.0f, neighbour->getFlipFlags()) -
		TileSet::flipTilePoint(part, 1.0f, tile->getFlipFlags());
	return offset.x == (float)dx && offset.y == (float)dy;
}

// How many more times a tile can crack, following its cracked_tile chain.
int cracksLeft(const Tile* tile)
{
	const TileSet::TileInfo* info = infoOf(tile);
	int count = 0;
	// The bound stops a chain that loops back on itself.
	while (info && info->hasProperty("cracked_tile") && count < 16) {
		++count;
		info = tile->getTileSet()->findTileInfo(info->getIntProperty("cracked_tile", -1));
	}
	return count;
}

bool breaks(const Tile* tile)
{
	return cracksLeft(tile) > 0;
}

// Touching breakable tiles are one prop when they sit the same way in the tileset image and
// can crack as many more times. So a 2x2 pot painted from the atlas joins up, while an
// intact pot painted beside its own cracked stage stays a separate prop.
bool sameBreakable(const Tile* tile, const Tile* neighbour, int dx, int dy)
{
	if (tile->getTileSet() != neighbour->getTileSet() || tile->getFlipFlags() != neighbour->getFlipFlags() ||
		cracksLeft(tile) != cracksLeft(neighbour)) {
		return false;
	}
	const int columns = std::max(1, (int)tile->getTileSet()->getTileCounts().x);
	const auto atlasCell = [columns](const Tile* part) {
		return vector2((float)(part->getTileIndex() % columns), (float)(part->getTileIndex() / columns));
	};
	return partsLineUp(tile, atlasCell(tile), neighbour, atlasCell(neighbour), dx, dy);
}

// Authored part coordinates keep each multi-tile platform together without joining
// neighbouring copies.
bool sameSinkingPlatform(const Tile* tile, const Tile* neighbour, int dx, int dy)
{
	const TileSet::TileInfo* info = infoOf(tile);
	const TileSet::TileInfo* neighbourInfo = infoOf(neighbour);
	const bool hasPart = info->hasProperty("platform_column") && info->hasProperty("platform_row");
	const bool neighbourHasPart = neighbourInfo->hasProperty("platform_column") && neighbourInfo->hasProperty("platform_row");
	if (!hasPart && !neighbourHasPart) {
		return true;
	}
	if (hasPart != neighbourHasPart || tile->getTileSet() != neighbour->getTileSet() ||
		tile->getFlipFlags() != neighbour->getFlipFlags()) {
		return false;
	}

	const auto partPosition = [](const TileSet::TileInfo* partInfo) {
		return vector2((float)partInfo->getIntProperty("platform_column", 0), (float)partInfo->getIntProperty("platform_row", 0));
	};
	return partsLineUp(tile, partPosition(info), neighbour, partPosition(neighbourInfo), dx, dy);
}

// World bounds of a tile's collider, or of its cell when it has none.
void tileBounds(Tile* tile, vector2& outMin, vector2& outMax)
{
	if (!Kinematics2D::tryGetActiveBounds(tile->getCollidable(), outMin, outMax)) {
		const float size = tile->getTileSet() ? tile->getTileSet()->getTileSize() : 16.0f;
		outMin = tile->getPosition();
		outMax = tile->getPosition() + vector2(size, size);
	}
}

// Grows min/max to take in the tile's bounds; empty stays true until the first tile.
void includeTileBounds(Tile* tile, bool& empty, vector2& min, vector2& max)
{
	vector2 tileMin, tileMax;
	tileBounds(tile, tileMin, tileMax);
	min = empty ? tileMin : vector2(std::min(min.x, tileMin.x), std::min(min.y, tileMin.y));
	max = empty ? tileMax : vector2(std::max(max.x, tileMax.x), std::max(max.y, tileMax.y));
	empty = false;
}

bool overlaps(const vector2& aMin, const vector2& aMax, const vector2& bMin, const vector2& bMax)
{
	return aMin.x < bMax.x && aMax.x > bMin.x && aMin.y < bMax.y && aMax.y > bMin.y;
}

// Groups tiles matching the predicate that touch horizontally or vertically.
std::vector<std::vector<Tile*>> connectedGroups(TileMap* layer, const std::function<bool(const Tile*)>& matches,
	const std::function<bool(const Tile*, const Tile*, int, int)>& joins = {})
{
	std::vector<std::vector<Tile*>> groups;
	const unsigned int width = layer->getMapWidth();
	const unsigned int height = layer->getMapHeight();
	std::vector<bool> visited((size_t)width * height, false);

	for (unsigned int y = 0; y < height; ++y) {
		for (unsigned int x = 0; x < width; ++x) {
			if (visited[(size_t)y * width + x] || !matches(layer->getTile(x, y))) {
				continue;
			}

			std::vector<Tile*> group;
			std::vector<std::pair<unsigned int, unsigned int>> open{ {x, y} };
			visited[(size_t)y * width + x] = true;
			while (!open.empty()) {
				const auto cell = open.back();
				open.pop_back();
				group.push_back(layer->getTile(cell.first, cell.second));

				const int neighbours[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
				for (const auto& offset : neighbours) {
					const int nx = (int)cell.first + offset[0];
					const int ny = (int)cell.second + offset[1];
					if (nx < 0 || ny < 0 || nx >= (int)width || ny >= (int)height) {
						continue;
					}
					const size_t index = (size_t)ny * width + (size_t)nx;
					Tile* neighbour = layer->getTile((unsigned int)nx, (unsigned int)ny);
					if (!visited[index] && matches(neighbour) &&
						(!joins || joins(layer->getTile(cell.first, cell.second), neighbour, offset[0], offset[1]))) {
						visited[index] = true;
						open.push_back({ (unsigned int)nx, (unsigned int)ny });
					}
				}
			}
			groups.push_back(std::move(group));
		}
	}
	return groups;
}
}

void LevelProps::initialize(const std::vector<TileMap*>& layers)
{
	clear();

	for (TileMap* layer : layers) {
		if (!layer) {
			continue;
		}

		for (const std::vector<Tile*>& group : connectedGroups(layer, [](const Tile* t) { return isClass(t, "key"); })) {
			for (Tile* tile : group) {
				_keys.push_back(Pickup{ tile, tile->getPosition() });
			}
		}
		for (std::vector<Tile*>& group : connectedGroups(layer, breaks, sameBreakable)) {
			Breakable breakable;
			breakable.tiles = std::move(group);
			_breakables.push_back(std::move(breakable));
		}

		for (std::vector<Tile*>& group : connectedGroups(layer, [](const Tile* t) { return isClass(t, "chest"); })) {
			Chest chest;
			chest.tiles = std::move(group);
			bool empty = true;
			for (Tile* tile : chest.tiles) {
				includeTileBounds(tile, empty, chest.min, chest.max);
				chest.heal = std::max(chest.heal, infoOf(tile)->getFloatProperty("heal", 0.0f));
			}
			_chests.push_back(std::move(chest));
		}

		for (std::vector<Tile*>& group : connectedGroups(layer, sinks, sameSinkingPlatform)) {
			SinkingPlatform platform;
			platform.tiles = std::move(group);
			for (Tile* tile : platform.tiles) {
				const TileSet::TileInfo* info = infoOf(tile);
				platform.restPositions.push_back(tile->getPosition());
				platform.sinkSpeed = std::max(platform.sinkSpeed, info->getFloatProperty("sink_speed", 0.0f));
				platform.riseSpeed = std::max(platform.riseSpeed, info->getFloatProperty("rise_speed", 0.0f));
				platform.maxDepth = std::max(platform.maxDepth, info->getFloatProperty("sink_depth", kDefaultSinkDepth));
			}
			if (platform.riseSpeed <= 0.0f) {
				platform.riseSpeed = platform.sinkSpeed * kDefaultRiseSpeedScale;
			}
			_platforms.push_back(std::move(platform));
		}

		const auto sameHazard = [](const Tile* tile, const Tile* neighbour, int, int) {
			return infoOf(tile)->_properties == infoOf(neighbour)->_properties;
		};
		for (std::vector<Tile*>& group : connectedGroups(layer, [](const Tile* t) { return isClass(t, "hazard"); }, sameHazard)) {
			LevelTriggerDescriptor hazard;
			hazard.layerId = layer->getLayerConfig().id;
			hazard.layerName = layer->getLayerConfig().name;
			hazard.typeName = "hazard";
			for (const auto& property : infoOf(group.front())->_properties) {
				hazard.properties.push_back(TriggerPropertyDescriptor{ property.first, "", property.second });
			}
			bool empty = true;
			vector2 min, max;
			for (Tile* tile : group) {
				includeTileBounds(tile, empty, min, max);
			}
			hazard.position = min;
			hazard.size = max - min;
			hazard.tiles = std::move(group);
			_hazards.push_back(std::move(hazard));
		}

		const TileMapPropertyDescriptor* openLayerName = layer->getLayerConfig().findProperty("open_layer");
		if (!openLayerName) {
			continue;
		}
		Door door;
		door.closedLayer = layer;
		for (TileMap* other : layers) {
			if (other && other != layer && other->getLayerConfig().name == openLayerName->value) {
				door.openLayer = other;
			}
		}
#if _DEBUG
		if (!door.openLayer) {
			const std::string message = "LevelProps: door layer '" + layer->getLayerConfig().name +
				"' names missing open_layer '" + openLayerName->value + "'\n";
			DEBUG_MSG(message.c_str());
		}
#endif
		bool empty = true;
		for (unsigned int y = 0; y < layer->getMapHeight(); ++y) {
			for (unsigned int x = 0; x < layer->getMapWidth(); ++x) {
				Tile* tile = layer->getTile(x, y);
				if (tile && tile->getTileIndex() >= 0) {
					includeTileBounds(tile, empty, door.min, door.max);
				}
			}
		}
		if (!empty) {
			_doors.push_back(door);
		}
	}
}

void LevelProps::clear(void)
{
	_keys.clear();
	_chests.clear();
	_doors.clear();
	_hazards.clear();
	_platforms.clear();
	_breakables.clear();
	_strikeAnimation = nullptr;
	_strikeFrame = -1;
	_keysHeld = 0;
	_lastEvent.clear();
}

void LevelProps::update(Character* character, bool interactPressed, float dt)
{
	vector2 bodyMin(0.0f, 0.0f), bodyMax(0.0f, 0.0f);
	const bool hasBody = character && character->getHealth() > 0.0f &&
		Kinematics2D::tryGetActiveBounds(character->getCollidable(), bodyMin, bodyMax);
	const bool falling = character && character->getVelocity().y >= -1.0f;
	const char* state = character && character->getState() ? character->getState()->getName() : "";
	const bool attacking = !std::strcmp(state, "Attack01") || !std::strcmp(state, "Attack02");
	auto* attackAnimation = attacking ? static_cast<Animation*>(character->getRenderable()) : nullptr;
	const bool strikeActive = attackAnimation && attackAnimation->getCurrentFrameIndex() > 0;
	const Animation* strikeAnimation = strikeActive ? attackAnimation : nullptr;
	const int strikeFrame = strikeActive ? (int)attackAnimation->getCurrentFrameIndex() : -1;
	if (strikeActive && (strikeAnimation != _strikeAnimation || strikeFrame < _strikeFrame)) {
		++_swing;
	}
	_strikeAnimation = strikeAnimation;
	_strikeFrame = strikeFrame;
	if (hasBody && strikeActive) {
		vector2 strikeMin = bodyMin, strikeMax = bodyMax;
		if (attackAnimation->getScale().x < 0.0f) { strikeMin.x -= 32.0f; }
		else { strikeMax.x += 32.0f; }
		for (Breakable& breakable : _breakables) {
			if (breakable.lastSwing == _swing) {
				continue;
			}
			const bool struck = std::any_of(breakable.tiles.begin(), breakable.tiles.end(), [&](Tile* tile) {
				vector2 tileMin, tileMax;
				tileBounds(tile, tileMin, tileMax);
				return overlaps(strikeMin, strikeMax, tileMin, tileMax);
			});
			if (!struck) {
				continue;
			}
			bool cracked = false;
			for (Tile* tile : breakable.tiles) {
				const TileSet::TileInfo* info = infoOf(tile);
				const int crackedTile = info ? info->getIntProperty("cracked_tile", -1) : -1;
				if (crackedTile >= 0) {
					tile->setTileIndex(crackedTile, tile->getFlipFlags());
					cracked = true;
				}
			}
			if (cracked) {
				breakable.lastSwing = _swing;
				_lastEvent = "prop_cracked";
			}
		}
	}

	if (hasBody) {
		for (Pickup& key : _keys) {
			if (key.collected) {
				continue;
			}
			vector2 keyMin, keyMax;
			tileBounds(key.tile, keyMin, keyMax);
			if (overlaps(bodyMin, bodyMax, keyMin, keyMax)) {
				key.collected = true;
				key.tile->setTileIndex(-1);
				++_keysHeld;
				_lastEvent = "key_collected";
#if _DEBUG
				DEBUG_MSG("LevelProps: key collected\n");
#endif
			}
			// Bob up from the painted cell and never below it, so a key resting on a
			// platform stays clear of it. Offsetting the rest position rather than the
			// last frame's keeps the motion the same at any frame rate.
			key.lastTime += dt;
			const float rise = kKeyBobHeight * 0.5f * (1.0f - std::cos(kKeyBobSpeed * key.lastTime));
			key.tile->setPosition(key.restPosition - vector2(0.0f, rise));
		}

		if (interactPressed) {
			const vector2 reach(kChestReach, kChestReach);
			for (Chest& chest : _chests) {
				if (chest.opened || !overlaps(bodyMin, bodyMax, chest.min - reach, chest.max + reach)) {
					continue;
				}
				// Holding a key is enough; it is not used up.
				if (_keysHeld <= 0) {
					_lastEvent = "chest_locked";
#if _DEBUG
					DEBUG_MSG("LevelProps: chest is locked (no key)\n");
#endif
					continue;
				}

				chest.opened = true;
				for (Tile* tile : chest.tiles) {
					const int openTile = infoOf(tile)->getIntProperty("open_tile", -1);
					if (openTile >= 0) {
						tile->setTileIndex(openTile, tile->getFlipFlags());
					}
				}
				if (chest.heal > 0.0f) {
					character->addHealth(chest.heal);
				}
				_lastEvent = "chest_opened";
#if _DEBUG
				DEBUG_MSG("LevelProps: chest opened\n");
#endif
			}

			for (Door& door : _doors) {
				if (door.opened || !overlaps(bodyMin, bodyMax, door.min - reach, door.max + reach)) {
					continue;
				}
				// As with chests, the key opens the door without being used up.
				if (_keysHeld <= 0) {
					_lastEvent = "door_locked";
#if _DEBUG
					DEBUG_MSG("LevelProps: door is locked (no key)\n");
#endif
					continue;
				}

				door.opened = true;
				door.closedLayer->setLayerVisibility(false);
				door.closedLayer->setLayerCollision(TileCollisionMode::None);
				if (door.openLayer) {
					door.openLayer->setLayerVisibility(true);
				}
				_lastEvent = "door_opened";
#if _DEBUG
				DEBUG_MSG("LevelProps: door opened\n");
#endif
			}
		}
	}

	// Choose support before moving any tiles, so a body spanning two pads can only
	// weigh down one. Prefer the closest surface, then the widest supporting overlap.
	SinkingPlatform* occupiedPlatform = nullptr;
	float bestDistance = std::numeric_limits<float>::max();
	float bestOverlap = 0.0f;
	constexpr float tieTolerance = 0.01f;
	if (hasBody && falling) {
		for (SinkingPlatform& platform : _platforms) {
			float distance = std::numeric_limits<float>::max();
			float overlap = 0.0f;
			for (Tile* tile : platform.tiles) {
				vector2 surfaceMin, surfaceMax;
				if (!character->shouldCollideWith(*tile) ||
					!Kinematics2D::tryGetActiveBounds(tile->getCollidable(), surfaceMin, surfaceMax)) {
					continue;
				}
				const float width = std::min(bodyMax.x, surfaceMax.x) - std::max(bodyMin.x, surfaceMin.x);
				float supportY = 0.0f;
				if (width <= 0.0f || !Kinematics2D::sampleSupportYInOverlap(
					tile->getCollidable(), bodyMin.x, bodyMax.x, supportY)) {
					continue;
				}
				const float footDistance = std::fabs(bodyMax.y - supportY);
				if (footDistance <= kStandTolerance) {
					distance = std::min(distance, footDistance);
					overlap += width;
				}
			}
			if (overlap <= 0.0f) {
				continue;
			}
			const bool sameDistance = std::fabs(distance - bestDistance) <= tieTolerance;
			const bool sameOverlap = std::fabs(overlap - bestOverlap) <= tieTolerance;
			if (!occupiedPlatform || distance < bestDistance - tieTolerance ||
				(sameDistance && (overlap > bestOverlap + tieTolerance ||
					(sameOverlap && platform.occupied && !occupiedPlatform->occupied)))) {
				occupiedPlatform = &platform;
				bestDistance = distance;
				bestOverlap = overlap;
			}
		}
	}
	for (SinkingPlatform& platform : _platforms) {
		updatePlatform(platform, &platform == occupiedPlatform, dt);
	}
}

void LevelProps::updatePlatform(SinkingPlatform& platform, bool occupied, float dt)
{
	platform.occupied = occupied;

	const float previousDepth = platform.depth;
	if (platform.occupied) {
		platform.depth = std::min(platform.maxDepth, platform.depth + platform.sinkSpeed * dt);
	}
	else {
		platform.depth = std::max(0.0f, platform.depth - platform.riseSpeed * dt);
	}

	if (platform.depth != previousDepth) {
		for (size_t i = 0; i < platform.tiles.size(); ++i) {
			platform.tiles[i]->setPosition(platform.restPositions[i] + vector2(0.0f, platform.depth));
		}
	}
}

void LevelProps::resetPlatforms(void)
{
	for (SinkingPlatform& platform : _platforms) {
		platform.occupied = false;
		if (platform.depth == 0.0f) {
			continue;
		}
		platform.depth = 0.0f;
		for (size_t i = 0; i < platform.tiles.size(); ++i) {
			platform.tiles[i]->setPosition(platform.restPositions[i]);
		}
	}
}
