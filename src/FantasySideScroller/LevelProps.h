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
//  - tiles of class "pot" crack when struck, swapping to "cracked_tile";
//  - tiles of class "chest" open when the character interacts while holding a key. Each
//    chest tile swaps to the tile id in its "open_tile" property; a "heal" property on any
//    of them restores that much health;
//  - tiles with a "sink_speed" property (pixels/second) form floating platforms that sink
//    while stood on and rise back when free ("rise_speed", "sink_depth" are optional).
// Touching tiles on the same layer act as one chest or platform. Platform tiles can
// specify "platform_column" and "platform_row" part coordinates to keep adjacent
// copies independent. Only one platform carries the character at a time.
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
	struct Pot {
		Tile* tile = nullptr;
		int crackedTile = -1;
		bool cracked = false;
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
	const std::vector<Pot>& getPots(void) const { return _pots; }

private:
	struct Pickup {
		Tile* tile = nullptr;
		bool collected = false;
		float floatOffset = 0.0f;
		float lastTime = 0.0f;
	};

	std::vector<Pickup> _keys;
	std::vector<Chest> _chests;
	std::vector<SinkingPlatform> _platforms;
	std::vector<Pot> _pots;
	int _keysHeld = 0;
	std::string _lastEvent;

	void updatePlatform(SinkingPlatform& platform, bool occupied, float dt);
};
