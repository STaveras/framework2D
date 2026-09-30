// LevelProps.h
#pragma once

#include "../Maths.h"

#include <string>
#include <vector>

class Character;
class Tile;
class TileMap;

// Gameplay driven entirely by tile data painted in Tiled, so levels need no extra objects:
//  - tiles of class "key" are picked up when the character touches them;
//  - tiles of class "chest" open when the character interacts while holding a key. Each
//    chest tile swaps to the tile id in its "open_tile" property; a "heal" property on any
//    of them restores that much health;
//  - tiles with a "sink_speed" property (pixels/second) form floating platforms that sink
//    while stood on and rise back when free ("rise_speed", "sink_depth" are optional).
// Tiles that touch each other on the same layer act as one chest or platform.
class LevelProps
{
public:
	struct Chest {
		std::vector<Tile*> tiles;
		vector2 min = vector2(0.0f, 0.0f);
		vector2 max = vector2(0.0f, 0.0f);
		float heal = 0.0f;
		bool opened = false;
	};

	struct SinkingPlatform {
		std::vector<Tile*> tiles;
		std::vector<vector2> restPositions;
		float sinkSpeed = 0.0f;
		float riseSpeed = 0.0f;
		float maxDepth = 0.0f;
		float depth = 0.0f;
		bool occupied = false;
	};

	void initialize(const std::vector<TileMap*>& layers);
	void clear(void);

	// interactPressed: the interact action went down this frame.
	void update(Character* character, bool interactPressed, float dt);
	// Returns every platform to the surface, e.g. after a respawn.
	void resetPlatforms(void);

	int getKeyCount(void) const { return _keysHeld; }
	const std::string& getLastEvent(void) const { return _lastEvent; }
	const std::vector<Chest>& getChests(void) const { return _chests; }
	const std::vector<SinkingPlatform>& getPlatforms(void) const { return _platforms; }

private:
	struct Pickup {
		Tile* tile = nullptr;
		bool collected = false;
	};

	std::vector<Pickup> _keys;
	std::vector<Chest> _chests;
	std::vector<SinkingPlatform> _platforms;
	int _keysHeld = 0;
	std::string _lastEvent;

	void updatePlatform(SinkingPlatform& platform, const vector2& bodyMin, const vector2& bodyMax,
		bool hasBody, bool falling, float dt);
};
