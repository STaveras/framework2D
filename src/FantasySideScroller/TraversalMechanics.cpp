#include "TraversalMechanics.h"

#include "Character.h"

#include "../Kinematics2D.h"
#include "../Square.h"
#include "../StrUtils.h"

#include <algorithm>
#include <cstdlib>

namespace
{
constexpr float kDefaultTriggerExtent = 8.0f;
}

bool TraversalMechanics::operator()(GameObject* object)
{
	if (!_trackedCharacter || !_runState.active || !object || object != _trackedCharacter) {
		return true;
	}

	const float dt = _frameDeltaSeconds;
	_frameDeltaSeconds = 0.0f;
	update(_trackedCharacter, dt);
	return true;
}

bool TraversalMechanics::overlapsCharacter(const TraversalTrigger& trigger, const Character& character)
{
	Collidable* body = ((Character&)character).getCollidable();
	vector2 bodyMin(0.0f, 0.0f);
	vector2 bodyMax(0.0f, 0.0f);
	if (!body || !Kinematics2D::tryGetActiveBounds(body, bodyMin, bodyMax)) {
		const vector2 characterPosition = character.getPosition();
		Square triggerBounds(trigger.position, trigger.position + trigger.size);
		return triggerBounds.collidesWith(characterPosition);
	}

	Square characterBounds(bodyMin, bodyMax);
	Square triggerBounds(trigger.position, trigger.position + trigger.size);
	return characterBounds.collidesWith(&triggerBounds);
}

TraversalTriggerType TraversalMechanics::parseTriggerType(const std::string& typeName)
{
	if (StrUtils::IEquals(typeName, "goal")) {
		return TraversalTriggerType::Goal;
	}
	if (StrUtils::IEquals(typeName, "checkpoint")) {
		return TraversalTriggerType::Checkpoint;
	}
	if (StrUtils::IEquals(typeName, "killzone")) {
		return TraversalTriggerType::Killzone;
	}
	if (StrUtils::IEquals(typeName, "time_bonus")) {
		return TraversalTriggerType::TimeBonus;
	}
	if (StrUtils::IEquals(typeName, "stamina_pickup")) {
		return TraversalTriggerType::StaminaPickup;
	}
	return TraversalTriggerType::Unknown;
}

float TraversalMechanics::readNumericProperty(const LevelTriggerDescriptor& descriptor, const char* key, float fallback)
{
	if (!key || key[0] == '\0') {
		return fallback;
	}

	for (const TriggerPropertyDescriptor& property : descriptor.properties) {
		if (!StrUtils::IEquals(property.name, key)) {
			continue;
		}

		char* endPtr = nullptr;
		const float parsedValue = std::strtof(property.value.c_str(), &endPtr);
		if (endPtr == property.value.c_str()) {
			return fallback;
		}
		return parsedValue;
	}

	return fallback;
}

bool TraversalMechanics::readBoolProperty(const LevelTriggerDescriptor& descriptor, const char* key, bool fallback)
{
	if (!key || key[0] == '\0') {
		return fallback;
	}

	for (const TriggerPropertyDescriptor& property : descriptor.properties) {
		if (!StrUtils::IEquals(property.name, key)) {
			continue;
		}
		return StrUtils::IsTruthy(property.value);
	}

	return fallback;
}

void TraversalMechanics::requestRespawn(const vector2& position, const char* reason)
{
	_respawnPending = true;
	_respawnPoint = position;
	_runState.failed = true;
	_runState.lastEvent = reason ? reason : "respawn";
}

void TraversalMechanics::initialize(
	const std::vector<LevelTriggerDescriptor>& descriptors,
	const vector2& spawnPoint,
	float timeLimitSeconds)
{
	_triggers.clear();

	for (const LevelTriggerDescriptor& descriptor : descriptors) {
		TraversalTrigger trigger;
		trigger.type = parseTriggerType(descriptor.typeName);
		if (trigger.type == TraversalTriggerType::Unknown) {
			continue;
		}

		trigger.name = descriptor.name;
		trigger.position = descriptor.position;
		trigger.size = descriptor.size;

		if (descriptor.isPoint || trigger.size.x <= 0.0f || trigger.size.y <= 0.0f) {
			trigger.position = descriptor.position - vector2(kDefaultTriggerExtent * 0.5f, kDefaultTriggerExtent * 0.5f);
			trigger.size = vector2(kDefaultTriggerExtent, kDefaultTriggerExtent);
		}

		switch (trigger.type) {
		case TraversalTriggerType::TimeBonus:
			trigger.value = readNumericProperty(descriptor, "seconds", readNumericProperty(descriptor, "value", 5.0f));
			trigger.oneShot = readBoolProperty(descriptor, "one_shot", true);
			break;
		case TraversalTriggerType::StaminaPickup:
			trigger.value = readNumericProperty(descriptor, "amount", readNumericProperty(descriptor, "value", 20.0f));
			trigger.oneShot = readBoolProperty(descriptor, "one_shot", true);
			break;
		case TraversalTriggerType::Checkpoint:
			trigger.oneShot = false;
			break;
		default:
			trigger.oneShot = true;
			break;
		}

		_triggers.push_back(trigger);
	}

	resetRun(spawnPoint, timeLimitSeconds);
}

void TraversalMechanics::resetRun(const vector2& spawnPoint, float timeLimitSeconds)
{
	if (timeLimitSeconds > 0.0f) {
		_runState.timeLimitSeconds = timeLimitSeconds;
	}
	else if (_runState.timeLimitSeconds <= 0.0f) {
		_runState.timeLimitSeconds = 75.0f;
	}

	_runState.active = true;
	_runState.completed = false;
	_runState.failed = false;
	_runState.elapsedSeconds = 0.0f;
	_runState.remainingSeconds = _runState.timeLimitSeconds;
	_runState.hasCheckpoint = false;
	_runState.spawnPoint = spawnPoint;
	_runState.checkpoint = spawnPoint;
	_runState.lastEvent = "run_started";
	_respawnPending = false;
	_respawnPoint = spawnPoint;

	for (TraversalTrigger& trigger : _triggers) {
		trigger.consumed = false;
	}
}

void TraversalMechanics::update(Character* character, float dt)
{
	if (!_runState.active || _runState.completed || !character) {
		return;
	}

	const float clampedDt = std::max(0.0f, dt);
	_runState.elapsedSeconds += clampedDt;
	_runState.remainingSeconds = std::max(0.0f, _runState.remainingSeconds - clampedDt);

	if (_runState.remainingSeconds <= 0.0f) {
		const vector2 resumePoint = _runState.hasCheckpoint ? _runState.checkpoint : _runState.spawnPoint;
		requestRespawn(resumePoint, "timeout");
		_runState.remainingSeconds = _runState.timeLimitSeconds;
	}

	if (_runState.completed) {
		return;
	}

	for (TraversalTrigger& trigger : _triggers) {
		if (trigger.oneShot && trigger.consumed) {
			continue;
		}

		if (!overlapsCharacter(trigger, *character)) {
			continue;
		}

		switch (trigger.type) {
		case TraversalTriggerType::Goal:
			_runState.completed = true;
			_runState.active = false;
			_runState.lastEvent = "goal_reached";
			break;
		case TraversalTriggerType::Checkpoint:
			_runState.hasCheckpoint = true;
			_runState.checkpoint = trigger.position + (trigger.size * 0.5f);
			_runState.lastEvent = "checkpoint";
			break;
		case TraversalTriggerType::Killzone: {
			const vector2 resumePoint = _runState.hasCheckpoint ? _runState.checkpoint : _runState.spawnPoint;
			requestRespawn(resumePoint, "killzone");
			break;
		}
		case TraversalTriggerType::TimeBonus:
			_runState.remainingSeconds += std::max(0.0f, trigger.value);
			_runState.lastEvent = "time_bonus";
			break;
		case TraversalTriggerType::StaminaPickup:
			character->addStamina(std::max(0.0f, trigger.value));
			_runState.lastEvent = "stamina_pickup";
			break;
		case TraversalTriggerType::Unknown:
			break;
		}

		if (trigger.oneShot) {
			trigger.consumed = true;
		}

		if (_runState.completed) {
			break;
		}
	}
}

bool TraversalMechanics::consumeRespawnRequest(vector2& outRespawnPoint)
{
	if (!_respawnPending) {
		return false;
	}

	_respawnPending = false;
	outRespawnPoint = _respawnPoint;
	return true;
}
