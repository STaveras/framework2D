// Headless regression checks for section exits and entry spawn selection on real maps.
#include "../src/Engine2D.h"
#include "../src/FantasySideScroller/Character.h"
#include "../src/FantasySideScroller/CharacterTuning.h"
#include "../src/FantasySideScroller/LevelManager.h"
#include "../src/FantasySideScroller/Resources.h"
#include "../src/FantasySideScroller/TraversalMechanics.h"
#include "../src/GameState.h"
#include "../src/Kinematics2D.h"
#include "../src/StrUtils.h"
#include "../src/System.h"
#include "../src/TileMap.h"
#include "stb/stb_image.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
class TestTexture : public ITexture {
	unsigned int width = 0, height = 0;
public:
	explicit TestTexture(const char* path) : ITexture(path) {
		int w = 0, h = 0, channels = 0;
		assert(stbi_info(path, &w, &h, &channels));
		width = w;
		height = h;
	}
	unsigned int getWidth() const override { return width; }
	unsigned int getHeight() const override { return height; }
};

class TestRenderer : public IRenderer {
public:
	ITexture* createTexture(const char* path, Color = 0) override {
		auto* texture = new TestTexture(path);
		m_Textures.store(texture);
		return texture;
	}
	void initialize() override {}
	void shutdown() override {}
	void render() override {}
};

void expectPosition(vector2 actual, vector2 expected) {
	if (std::fabs(actual.x - expected.x) > 0.01f || std::fabs(actual.y - expected.y) > 0.01f) {
		std::cerr << "expected spawn {" << expected.x << ',' << expected.y
			<< "}, got {" << actual.x << ',' << actual.y << "}\n";
		assert(false);
	}
}

// The map's spawn markers as authored, read straight from the file, and where each
// should put the character: its feet on the marker.
struct SpawnMarkers {
	std::vector<vector2> origins;
	float midX = 0.0f;

	// Spawns at the map's left or right end, top to bottom.
	std::vector<vector2> atEnd(bool right) const {
		std::vector<vector2> end;
		for (const vector2& origin : origins) {
			if ((origin.x > midX) == right) end.push_back(origin);
		}
		std::sort(end.begin(), end.end(), [](const vector2& a, const vector2& b) { return a.y < b.y; });
		return end;
	}
};

SpawnMarkers readSpawnMarkers(const char* mapFile, vector2 mapOffset) {
	TileMapLoadResult map = TileMap::loadMapDataFromJSONFile(BasePath(mapFile).c_str(), nullptr, false);
	SpawnMarkers spawns;
	spawns.midX = mapOffset.x + map.mapWidth * map.tileWidth * 0.5f;
	for (const TileObjectLayerDescriptor& layer : map.objectLayers) {
		for (const TileObjectDescriptor& object : layer.objects) {
			if (StrUtils::IEquals(object.typeName, "spawn")) {
				spawns.origins.push_back(mapOffset + vector2(object.x, object.y - kLocomotionFootLocalY));
			}
		}
	}
	assert(!spawns.origins.empty());
	return spawns;
}

// Touches the named section boundary on the loaded map, returning the next map
// load it requests. Y is the character's outgoing height.
std::string exitThrough(LevelManager& level, TraversalMechanics& mechanics, Character& hero,
	const char* boundaryName, float y, std::string& outEntryName, float& outCharacterY) {
	mechanics.initialize(level.getTriggerDescriptors(), level.getSpawnPoint());
	hero.setPosition(level.getSpawnPoint());
	mechanics.update(&hero, 0.0f);
	const TraversalTrigger* exit = nullptr;
	for (const auto& trigger : mechanics.getTriggers()) {
		if (StrUtils::IEquals(trigger.name, boundaryName)) exit = &trigger;
	}
	assert(exit);
	hero.setPosition(exit->position.x + exit->size.x * 0.5f, y);
	mechanics.update(&hero, 0.0f);
	std::string nextMap;
	assert(mechanics.consumeMapChangeRequest(nextMap, &outEntryName, &outCharacterY));
	assert(!mechanics.consumeMapChangeRequest(nextMap));
	return nextMap;
}

// Standing at the entry spawn must not touch a destination and leave again.
void expectNoExitAt(LevelManager& level, TraversalMechanics& mechanics, Character& hero, vector2 spawn) {
	mechanics.initialize(level.getTriggerDescriptors(), spawn);
	hero.setPosition(spawn);
	vector2 bodyMin, bodyMax;
	assert(Kinematics2D::tryGetActiveBounds(hero.getCollidable(), bodyMin, bodyMax));
	mechanics.update(&hero, 0.0f);
	std::string nextMap;
	assert(!mechanics.consumeMapChangeRequest(nextMap));
}
}

int main() {
	System::GlobalDataPath("bin/fantasySideScroller");
	TestRenderer renderer;
	Engine2D::setRenderer(&renderer);
	GameState state;
	ObjectManager& objects = *state.getObjectManager();
	LevelManager level;
	TraversalMechanics mechanics;
	Character hero;
	hero.setState("Idle");
	const vector2 mapOffset(-60.0f, 0.0f);
	const SpawnMarkers forest = readSpawnMarkers("mosswood_hollow.tmj", mapOffset);
	const SpawnMarkers mine = readSpawnMarkers("old_mine_trail.tmj", mapOffset);
	std::string entryName;
	float characterY = 0.0f;

	// Forward: the forest's SectionEnd enters the mine beside its SectionBegin.
	level.initialize("mosswood_hollow.tmj", mapOffset, nullptr, objects, state);
	expectPosition(level.getSpawnPoint(), forest.origins.front());
	const float forestExitY = forest.atEnd(true).back().y;
	std::string nextMap = exitThrough(level, mechanics, hero, "SectionEnd", forestExitY, entryName, characterY);
	assert(nextMap == "old_mine_trail.tmj");
	assert(entryName == "SectionBegin" && characterY == forestExitY);
	level.shutdown(objects, state);

	// Enter at the mine's SectionBegin, even at the far-end spawn's exact height, rather
	// than spawning beside its SectionEnd and immediately leaving again.
	level.initialize(nextMap.c_str(), mapOffset, nullptr, objects, state);
	expectPosition(level.getSpawnPoint(), mine.origins.front());
	assert(mine.atEnd(false).size() == 1 && mine.atEnd(true).size() == 1);
	const vector2 mineStart = mine.atEnd(false).front();
	const vector2 mineFarEnd = mine.atEnd(true).front();
	expectPosition(level.getSpawnPointNearDestination(entryName, characterY), mineStart);
	expectPosition(level.getSpawnPointNearDestination("SectionBegin", mineFarEnd.y), mineStart);
	expectPosition(level.getSpawnPointNearDestination("SectionEnd", mineStart.y), mineFarEnd);
	for (const vector2& spawn : mine.origins) {
		expectNoExitAt(level, mechanics, hero, spawn);
	}

	// Reverse: the mine's SectionBegin enters the forest beside its SectionEnd.
	const float mineExitY = mineStart.y;
	nextMap = exitThrough(level, mechanics, hero, "SectionBegin", mineExitY, entryName, characterY);
	assert(nextMap == "mosswood_hollow.tmj");
	assert(entryName == "SectionEnd" && characterY == mineExitY);
	level.shutdown(objects, state);

	// The forest has entry and exit spawns at different heights at both ends; the
	// entry marker supplies X, and the character supplies Y, for the closest spawn.
	level.initialize(nextMap.c_str(), mapOffset, nullptr, objects, state);
	for (const bool rightEnd : { false, true }) {
		const std::vector<vector2> end = forest.atEnd(rightEnd);
		assert(end.size() == 2);
		for (const vector2& spawn : end) {
			expectPosition(level.getSpawnPointNearDestination(rightEnd ? "SectionEnd" : "SectionBegin", spawn.y), spawn);
		}
	}
	for (const vector2& spawn : forest.origins) {
		expectNoExitAt(level, mechanics, hero, spawn);
	}

	// A run that starts touching SectionEnd holds it until the character steps off it,
	// then fires on the next touch.
	{
		const TraversalTrigger* sectionEnd = nullptr;
		mechanics.initialize(level.getTriggerDescriptors(), level.getSpawnPoint());
		for (const auto& trigger : mechanics.getTriggers()) {
			if (StrUtils::IEquals(trigger.name, "SectionEnd")) sectionEnd = &trigger;
		}
		assert(sectionEnd);
		const vector2 touching(sectionEnd->position.x - 10.0f, 300.0f);
		mechanics.initialize(level.getTriggerDescriptors(), touching);
		hero.setPosition(touching);
		mechanics.update(&hero, 0.0f);
		hero.setPosition(touching + vector2(1, 0));
		mechanics.update(&hero, 0.0f);
		assert(!mechanics.consumeMapChangeRequest(nextMap));
		hero.setPosition(touching - vector2(8, 0));
		mechanics.update(&hero, 0.0f);
		assert(!mechanics.consumeMapChangeRequest(nextMap));
		hero.setPosition(touching);
		mechanics.update(&hero, 0.0f);
		assert(mechanics.consumeMapChangeRequest(nextMap, &entryName) && entryName == "SectionBegin");
	}

	// Reverse from the forest: its SectionBegin enters the mine beside its SectionEnd.
	nextMap = exitThrough(level, mechanics, hero, "SectionBegin", forest.atEnd(false).back().y, entryName, characterY);
	assert(nextMap == "old_mine_trail.tmj");
	assert(entryName == "SectionEnd");
	level.shutdown(objects, state);
	level.initialize(nextMap.c_str(), mapOffset, nullptr, objects, state);
	expectPosition(level.getSpawnPointNearDestination(entryName, characterY), mineFarEnd);

	// A map without the named destination falls back to its first spawn.
	expectPosition(level.getSpawnPointNearDestination("NoSuchDestination", 0), level.getSpawnPoint());
	level.shutdown(objects, state);

	std::cout << "section transition test passed\n";
}
