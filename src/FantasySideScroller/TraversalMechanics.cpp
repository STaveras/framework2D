#include "TraversalMechanics.h"

#include "Character.h"

#include "../Kinematics2D.h"
#include "../Square.h"
#include "../StrUtils.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr float kDefaultTriggerExtent = 8.0f;
constexpr float kDefaultHazardDamage = 10.0f;
constexpr float kDefaultHazardInterval = 1.0f;
constexpr float kDefaultHazardPushSpeed = 190.0f;
constexpr float kDefaultHazardPushLock = 0.3f;
// Sideways pushes also lift the character so it leaves the ground.
constexpr float kHazardSidePushLift = 0.55f;
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
	if (StrUtils::IEquals(typeName, "destination")) {
		return TraversalTriggerType::Destination;
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
	if (StrUtils::IEquals(typeName, "hazard")) {
		return TraversalTriggerType::Hazard;
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

std::string TraversalMechanics::readStringProperty(const LevelTriggerDescriptor& descriptor, const char* key)
{
	for (const TriggerPropertyDescriptor& property : descriptor.properties) {
		if (StrUtils::IEquals(property.name, key)) {
			return property.value;
		}
	}
	return std::string();
}

void TraversalMechanics::requestRespawn(const vector2& position, const char* reason)
{
	_respawnPending = true;
	_respawnPoint = position;
	_respawnReason = reason ? reason : "respawn";
	_runState.failed = true;
	_runState.lastEvent = reason ? reason : "respawn";
}

void TraversalMechanics::requestMapChange(const std::string& nextMapFileName)
{
	_mapChangePending = true;
	_nextMapFileName = nextMapFileName;
	_runState.completed = true;
	_runState.active = false;
	_runState.lastEvent = "destination_reached";
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
		case TraversalTriggerType::Destination:
			// next_map names the relative path of the map to load when reached.
			trigger.nextMap = readStringProperty(descriptor, "next_map");
			trigger.oneShot = true;
			break;
		case TraversalTriggerType::Checkpoint:
		case TraversalTriggerType::Killzone:
			// Both must keep working however many times the player passes or falls in.
			trigger.oneShot = false;
			break;
		case TraversalTriggerType::Hazard:
			trigger.value = readNumericProperty(descriptor, "damage", kDefaultHazardDamage);
			trigger.interval = std::max(0.0f, readNumericProperty(descriptor, "interval", kDefaultHazardInterval));
			trigger.oneShot = false;
			{
				// "push" names the direction the hazard points, and so throws the character.
				const std::string direction = readStringProperty(descriptor, "push");
				const float speed = readNumericProperty(descriptor, "push_speed", kDefaultHazardPushSpeed);
				if (StrUtils::IEquals(direction, "left")) {
					trigger.push = vector2(-speed, -speed * kHazardSidePushLift);
				}
				else if (StrUtils::IEquals(direction, "right")) {
					trigger.push = vector2(speed, -speed * kHazardSidePushLift);
				}
				else if (StrUtils::IEquals(direction, "up")) {
					trigger.push = vector2(0.0f, -speed);
				}
				else if (StrUtils::IEquals(direction, "down")) {
					trigger.push = vector2(0.0f, speed);
				}
				else if (StrUtils::IEquals(direction, "away")) {
					trigger.push = vector2(speed, -speed * kHazardSidePushLift);
					trigger.pushAway = true;
				}
				trigger.pushLockSeconds = readNumericProperty(descriptor, "push_lock", kDefaultHazardPushLock);
			}
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
	_mapChangePending = false;
	_nextMapFileName.clear();

	for (TraversalTrigger& trigger : _triggers) {
		trigger.consumed = false;
		trigger.cooldownRemaining = 0.0f;
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
		trigger.cooldownRemaining = std::max(0.0f, trigger.cooldownRemaining - clampedDt);
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
		case TraversalTriggerType::Destination:
			if (!trigger.nextMap.empty()) {
				// A destination with a next_map loads that map instead of ending the run.
				requestMapChange(trigger.nextMap);
			}
			else {
				// No next_map: treat it as a run goal so a terminal destination still finishes.
				_runState.completed = true;
				_runState.active = false;
				_runState.lastEvent = "goal_reached";
			}
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
		case TraversalTriggerType::Hazard:
			if (character->getHealth() <= 0.0f) {
				break;
			}
			// Damage waits for the interval; the push applies on every touch, so a hazard also
			// works as a barrier rather than letting the character through between hits.
			if (trigger.cooldownRemaining <= 0.0f) {
				character->addHealth(-std::max(0.0f, trigger.value));
				trigger.cooldownRemaining = trigger.interval;
				_runState.lastEvent = "hazard";
				if (character->getHealth() <= 0.0f) {
					// A lethal hit kills the character, as a boar's final bite does.
					character->setState("Dead");
					break;
				}
			}
			if ((trigger.push.x != 0.0f || trigger.push.y != 0.0f) && !character->isKnockedBack()) {
				vector2 push = trigger.push;
				if (trigger.pushAway) {
					vector2 bodyMin(0.0f, 0.0f), bodyMax(0.0f, 0.0f);
					float bodyCenterX = character->getPosition().x;
					if (Kinematics2D::tryGetActiveBounds(character->getCollidable(), bodyMin, bodyMax)) {
						bodyCenterX = (bodyMin.x + bodyMax.x) * 0.5f;
					}
					const float triggerCenterX = trigger.position.x + trigger.size.x * 0.5f;
					push.x = (bodyCenterX < triggerCenterX) ? -std::fabs(push.x) : std::fabs(push.x);
				}
				character->applyKnockback(push, trigger.pushLockSeconds);
			}
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

bool TraversalMechanics::consumeRespawnRequest(vector2& outRespawnPoint, std::string* outReason)
{
	if (!_respawnPending) {
		return false;
	}

	_respawnPending = false;
	outRespawnPoint = _respawnPoint;
	if (outReason) {
		*outReason = _respawnReason;
	}
	return true;
}

bool TraversalMechanics::consumeMapChangeRequest(std::string& outNextMapFileName)
{
	if (!_mapChangePending) {
		return false;
	}

	_mapChangePending = false;
	outNextMapFileName = _nextMapFileName;
	return true;
}
