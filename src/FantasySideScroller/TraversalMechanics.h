#pragma once

#include "../Maths.h"
#include "../ObjectOperator.h"
#include "../Trigger.h"

#include <string>
#include <vector>

class GameObject;
class Character;

struct TraversalRunState
{
	bool active = false;
	bool completed = false;
	bool failed = false;
	float elapsedSeconds = 0.0f;
	float remainingSeconds = 0.0f;
	float timeLimitSeconds = 0.0f;
	bool hasCheckpoint = false;
	vector2 spawnPoint = vector2(0.0f, 0.0f);
	vector2 checkpoint = vector2(0.0f, 0.0f);
	std::string lastEvent;
};

class TraversalMechanics : public ObjectOperator
{
	std::vector<TraversalTrigger> _triggers;
	TraversalRunState _runState;
	bool _respawnPending = false;
	vector2 _respawnPoint = vector2(0.0f, 0.0f);
	Character* _trackedCharacter = NULL;
	float _frameDeltaSeconds = 0.0f;

	static bool overlapsCharacter(const TraversalTrigger& trigger, const Character& character);
	static TraversalTriggerType parseTriggerType(const std::string& typeName);
	static float readNumericProperty(const LevelTriggerDescriptor& descriptor, const char* key, float fallback);
	static bool readBoolProperty(const LevelTriggerDescriptor& descriptor, const char* key, bool fallback);

	void requestRespawn(const vector2& position, const char* reason);

public:
	bool operator()(GameObject* object) override;

	void setTrackedCharacter(Character* character) { _trackedCharacter = character; }
	Character* getTrackedCharacter(void) const { return _trackedCharacter; }
	void setFrameDeltaSeconds(float dt) { _frameDeltaSeconds = dt; }

	void initialize(
		const std::vector<LevelTriggerDescriptor>& descriptors,
		const vector2& spawnPoint,
		float timeLimitSeconds = 75.0f);
	void resetRun(const vector2& spawnPoint, float timeLimitSeconds = -1.0f);
	void update(Character* character, float dt);
	bool consumeRespawnRequest(vector2& outRespawnPoint);

	const TraversalRunState& getRunState(void) const { return _runState; }
	const std::vector<TraversalTrigger>& getTriggers(void) const { return _triggers; }
};
