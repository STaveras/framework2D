// Character.cpp

#include "Character.h"

#include "../Animation.h"
#include "../CollidableGroup.h"
#include "../GameState.h"
#include "../Polygon.h"
#include "../Square.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>

namespace {
constexpr float kGroundLossGraceSeconds = 0.06f;
constexpr float kDropThroughDurationSeconds = 0.20f;
constexpr float kSupportSampleInset = 2.0f;
constexpr float kDefaultMaxSnapPerFrame = 8.0f;
constexpr float kGroundNormalThreshold = 0.2f;
constexpr float kOneWayTopApproachEpsilon = 1.0f;
constexpr float kLocomotionFootLocalY = 24.0f;
constexpr float kFootlineTolerance = 0.5f;
constexpr float kFootlineEpsilon = 0.001f;
constexpr float kWallNormalThreshold = 0.55f;
constexpr float kHorizontalSeparationEpsilon = 0.01f;
constexpr float kMaxHorizontalSeparationPerContact = 4.0f;
constexpr float kStepUpAssistEpsilon = 0.05f;
constexpr float kUpwardSnapMultiplier = 2.0f;
constexpr float kUpwardSupportBias = 0.25f;
constexpr float kSlopePriorityEpsilon = 0.25f;
constexpr float kSlopeFootClearance = 0.1f;
constexpr float kSupportSwitchHysteresis = 0.35f;
constexpr float kUphillProbeDistance = 1.5f;
constexpr float kUphillProbeMaxRise = 4.0f;
constexpr float kWalkMaxHorizontalSpeed = 95.0f;
constexpr float kRunMaxHorizontalSpeed = 130.0f;
constexpr float kAirMaxHorizontalSpeed = 112.0f;
constexpr float kWalkGroundAcceleration = 650.0f;
constexpr float kRunGroundAcceleration = 860.0f;
constexpr float kGroundTurnAcceleration = 1300.0f;
constexpr float kGroundDeceleration = 1450.0f;
constexpr float kAirAcceleration = 360.0f;
constexpr float kAirTurnAcceleration = 520.0f;
constexpr float kAirDeceleration = 280.0f;
constexpr float kStaminaMax = 100.0f;
constexpr float kStaminaDrainPerSecond = 25.0f;
constexpr float kStaminaRegenPerSecond = 40.0f;
constexpr float kRunAnimationSpeed = 1.1f;
constexpr float kRunBoostAnimationSpeed = 1.35f;
constexpr float kHorizontalVelocityEpsilon = 0.01f;
constexpr float kHorizontalSnapTravelPadding = 2.0f;
constexpr float kFastFallAcceleration = 900.0f;
constexpr float kFastFallMaxSpeed = 300.0f;
constexpr float kLongJumpLaunchSpeedThreshold = 110.0f;
constexpr float kLongJumpMomentumDecayPerSecond = 45.0f;

bool isSquareOnlyCollidable(const Collidable* collidable)
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	switch (collidable->getType()) {
	case COL_OBJ_SQUARE:
		return true;
	case COL_OBJ_GROUP: {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group || group->empty()) {
			return false;
		}

		for (const Collidable* member : *group) {
			if (!isSquareOnlyCollidable(member)) {
				return false;
			}
		}
		return true;
	}
	default:
		return false;
	}
}
}

Character::Character(void) : 
	GameObject(GAME_OBJ_OBJECT),
	_tile(NULL) {

	// TODO: Write a loadObjectFromJSON function to load the character from a JSON file, otherwise, call the init functions directly
	//		 Basically, it calls

	_initStates();
	_initTransitions();

	this->setBuffered(false);
	this->setState("Falling");
	this->setMass(100);
	_maxStamina = kStaminaMax;
	_stamina = _maxStamina;
	_runBoostActive = false;
}

Character::~Character() {}

bool Character::_isOneWayTile(const Tile* tile) const
{
	return tile &&
		tile->getTileType() == "tile" &&
		tile->isOneWay();
}

bool Character::_isDropThroughRequested() const
{
	Game* game = Engine2D::getGame();
	if (!game) {
		return false;
	}

	Player* player = game->getPlayerWith((GameObject*)this);
	if (!player || !player->getController()) {
		return false;
	}

	Controller* controller = player->getController();
	Action* jumpAction = controller->getAction("JUMP");
	Action* downAction = controller->getAction("DOWN");
	if (!jumpAction || !downAction) {
		return false;
	}

	return controller->buttonPressed(jumpAction) && controller->buttonDown(downAction);
}

void Character::_startDropThrough()
{
	_dropThroughTimer = kDropThroughDurationSeconds;

	for (auto itr = _groundContacts.begin(); itr != _groundContacts.end();) {
		Tile* tile = *itr;
		if (_isOneWayTile(tile)) {
			itr = _groundContacts.erase(itr);
			continue;
		}
		++itr;
	}

	_refreshGroundTile();
	_timeWithoutGroundContact = kGroundLossGraceSeconds;

	if (GameObjectState* state = this->getState()) {
		const char* stateName = state->getName();
		if (_isGroundedLocomotionState(stateName) || !strcmp(stateName, "Attack01") || !strcmp(stateName, "Attack02")) {
			this->sendInput("IN_AIR");
		}
	}
}

bool Character::_canCollideWithOneWayTile(const Tile* tile) const
{
	if (!_isOneWayTile(tile)) {
		return true;
	}

	if (_dropThroughTimer > 0.0f) {
		return false;
	}

	if (this->getVelocity().y < 0.0f) {
		return false;
	}

	Collidable* selfCollidable = ((Character*)this)->getCollidable();
	Collidable* tileCollidable = ((Tile*)tile)->getCollidable();
	if (!selfCollidable || !tileCollidable || !selfCollidable->isActive() || !tileCollidable->isActive()) {
		return true;
	}

	if (selfCollidable->getType() != COL_OBJ_SQUARE) {
		return true;
	}

	Square* bodySquare = (Square*)selfCollidable;
	const vector2 bodyMin = bodySquare->getMin();
	const vector2 bodyMax = bodySquare->getMax();
	const float sampleX = bodyMin.x + ((bodyMax.x - bodyMin.x) * 0.5f);

	float supportY = 0.0f;
	if (!_sampleSupportY(tileCollidable, sampleX, supportY)) {
		return true;
	}

	const float bodyTop = bodyMin.y;
	return bodyTop < (supportY + kOneWayTopApproachEpsilon);
}

bool Character::shouldCollideWith(const GameObject& other) const
{
	if (!GameObject::shouldCollideWith(other)) {
		return false;
	}

	if (other.getType() != GAME_OBJ_TILE) {
		return true;
	}

	const Tile* tile = (const Tile*)(&other);
	if (!tile) {
		return false;
	}

	if (tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile)) {
		return _canCollideWithOneWayTile(tile);
	}

	return true;
}

bool Character::_isGroundContact(const CollisionContact& contact) const
{
	if (!contact.other || contact.phase == CollisionPhase::Exit || contact.other->getType() != GAME_OBJ_TILE) {
		return false;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile || tile->getTileType() != "tile") {
		return false;
	}

	if (tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
		return false;
	}

	if (contact.normal.has_value()) {
		return contact.normal->y > kGroundNormalThreshold;
	}

	vector2 directionToObject = tile->getPosition() - this->getPosition();
	directionToObject.normalize();
	return directionToObject.y > 0.0f;
}

bool Character::_isGroundedLocomotionState(const char* stateName) const
{
	if (!stateName) {
		return false;
	}

	return !strcmp(stateName, "Idle") ||
		!strcmp(stateName, "RunningLeft") ||
		!strcmp(stateName, "RunningRight") ||
		!strcmp(stateName, "Landing");
}

int Character::_getHorizontalIntent() const
{
	const int horizontalInput = _getHorizontalInput();
	if (horizontalInput != 0) {
		return horizontalInput;
	}

	const float vx = this->getVelocity().x;
	if (vx > 0.001f) {
		return 1;
	}
	if (vx < -0.001f) {
		return -1;
	}

	return 0;
}

int Character::_getHorizontalInput() const
{
	Game* game = Engine2D::getGame();
	if (!game) {
		return 0;
	}

	Player* player = game->getPlayerWith((GameObject*)this);
	if (!player || !player->getController()) {
		return 0;
	}

	Controller* controller = player->getController();
	Action* leftAction = controller->getAction("LEFT");
	Action* rightAction = controller->getAction("RIGHT");
	const bool leftActive = leftAction && leftAction->isActive();
	const bool rightActive = rightAction && rightAction->isActive();

	if (leftActive == rightActive) {
		return 0;
	}

	return rightActive ? 1 : -1;
}

bool Character::_isRunRequested() const
{
	Game* game = Engine2D::getGame();
	if (!game) {
		return false;
	}

	Player* player = game->getPlayerWith((GameObject*)this);
	if (!player || !player->getController()) {
		return false;
	}

	Controller* controller = player->getController();
	Action* runAction = controller->getAction("RUN");
	return runAction && runAction->isActive();
}

bool Character::_getStateFootLocalY(const GameObjectState* state, float& outFootY) const
{
	outFootY = 0.0f;
	if (!state) {
		return false;
	}

	const GameObjectState* resolvedState = state;
	Collidable* stateCollidable = const_cast<GameObjectState*>(resolvedState)->getCollidable();
	if (!stateCollidable) {
		return false;
	}

	std::function<bool(const Collidable*, float, float&)> findFootLocalY =
		[&](const Collidable* collidable, float parentOffsetY, float& outFootLocalY) -> bool {
			if (!collidable) {
				return false;
			}

			switch (collidable->getType()) {
			case COL_OBJ_SQUARE: {
				const Square* square = (const Square*)collidable;
				if (!square) {
					return false;
				}

				outFootLocalY = parentOffsetY + square->getPosition().y + square->getHeight();
				return true;
			}
			case COL_OBJ_POLYGON: {
				const PolygonCollider* polygon = (const PolygonCollider*)collidable;
				if (!polygon || !polygon->isValid()) {
					return false;
				}

				const std::vector<vector2>& localVertices = polygon->getLocalVertices();
				if (localVertices.empty()) {
					return false;
				}

				float maxVertexY = std::numeric_limits<float>::lowest();
				for (const vector2& vertex : localVertices) {
					if (vertex.y > maxVertexY) {
						maxVertexY = vertex.y;
					}
				}

				outFootLocalY = parentOffsetY + polygon->getPosition().y + maxVertexY;
				return true;
			}
			case COL_OBJ_GROUP: {
				const CollidableGroup* group = (const CollidableGroup*)collidable;
				if (!group) {
					return false;
				}

				const float groupOffsetY = parentOffsetY + group->getPosition().y;
				bool hasMember = false;
				float bestMemberFootY = std::numeric_limits<float>::lowest();
				for (const Collidable* member : *group) {
					float memberFootY = 0.0f;
					if (!findFootLocalY(member, groupOffsetY, memberFootY)) {
						continue;
					}

					if (!hasMember || memberFootY > bestMemberFootY) {
						bestMemberFootY = memberFootY;
						hasMember = true;
					}
				}

				if (!hasMember) {
					return false;
				}

				outFootLocalY = bestMemberFootY;
				return true;
			}
			default:
				return false;
			}
		};

	return findFootLocalY(stateCollidable, 0.0f, outFootY);
}

void Character::_refreshGroundTile()
{
	Tile* bestTile = NULL;
	float bestDistance = std::numeric_limits<float>::max();
	std::vector<Tile*> staleTiles;

	for (Tile* tile : _groundContacts) {
		if (!tile || tile->getTileType() != "tile") {
			staleTiles.push_back(tile);
			continue;
		}

		if (tile->isNonCollidingLayer()) {
			staleTiles.push_back(tile);
			continue;
		}

		if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
			staleTiles.push_back(tile);
			continue;
		}

		Collidable* collidable = tile->getCollidable();
		if (!collidable || !collidable->isActive()) {
			staleTiles.push_back(tile);
			continue;
		}

		vector2 delta(
			tile->getPosition().x - this->getPosition().x,
			tile->getPosition().y - this->getPosition().y);
		float distance = delta.norm();
		if (distance < bestDistance) {
			bestDistance = distance;
			bestTile = tile;
		}
	}

	for (Tile* staleTile : staleTiles) {
		_groundContacts.erase(staleTile);
	}

	_tile = bestTile;
}

bool Character::_sampleSupportY(const Collidable* collidable, float sampleX, float& outY) const
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	constexpr float kHorizontalEpsilon = 0.001f;
	switch (collidable->getType()) {
	case COL_OBJ_SQUARE: {
		const Square* square = (const Square*)collidable;
		if (!square) {
			return false;
		}

		const vector2 min = square->getMin();
		const vector2 max = square->getMax();
		if (sampleX < (min.x - kHorizontalEpsilon) || sampleX > (max.x + kHorizontalEpsilon)) {
			return false;
		}

		outY = min.y;
		return true;
	}
	case COL_OBJ_POLYGON: {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (!polygon || !polygon->isValid()) {
			return false;
		}
		return polygon->findTopSurfaceYAtX(sampleX, outY);
	}
	case COL_OBJ_GROUP: {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group) {
			return false;
		}

		bool found = false;
		float bestY = std::numeric_limits<float>::max();
		for (const Collidable* member : *group) {
			float memberY = 0.0f;
			if (!_sampleSupportY(member, sampleX, memberY)) {
				continue;
			}

			if (!found || memberY < bestY) {
				bestY = memberY;
				found = true;
			}
		}

		if (!found) {
			return false;
		}

		outY = bestY;
		return true;
	}
	default:
		return false;
	}
}

bool Character::_findSupportOnTile(const Tile* tile, float footY, float maxSnapDistance, float& outSupportY) const
{
	outSupportY = footY;
	if (!tile || tile->getTileType() != "tile" || tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
		return false;
	}

	Collidable* body = ((Character*)this)->getCollidable();
	if (!body || !body->isActive() || body->getType() != COL_OBJ_SQUARE) {
		return false;
	}

	Collidable* tileCollidable = ((Tile*)tile)->getCollidable();
	if (!tileCollidable || !tileCollidable->isActive()) {
		return false;
	}

	Square* bodySquare = (Square*)body;
	const vector2 bodyMin = bodySquare->getMin();
	const vector2 bodyMax = bodySquare->getMax();
	const float bodyWidth = bodyMax.x - bodyMin.x;
	if (bodyWidth <= 0.0f) {
		return false;
	}

	float sampleInset = std::min(kSupportSampleInset, bodyWidth * 0.45f);
	if (sampleInset < 0.0f) {
		sampleInset = 0.0f;
	}

	const float sampleXs[3] = {
		bodyMin.x + sampleInset,
		bodyMin.x + (bodyWidth * 0.5f),
		bodyMax.x - sampleInset
	};

	struct TileSample {
		bool hasSupport = false;
		float supportY = 0.0f;
		float deltaY = 0.0f;
	};
	TileSample samples[3];

	const float maxUpwardSnapDistance = std::max(maxSnapDistance, maxSnapDistance * kUpwardSnapMultiplier);
	const float maxDownwardSnapDistance = maxSnapDistance;
	for (int sampleIndex = 0; sampleIndex < 3; ++sampleIndex) {
		float supportY = 0.0f;
		if (!_sampleSupportY(tileCollidable, sampleXs[sampleIndex], supportY)) {
			continue;
		}

		const float deltaY = supportY - footY;
		if (deltaY < -maxUpwardSnapDistance || deltaY > maxDownwardSnapDistance) {
			continue;
		}

		samples[sampleIndex].hasSupport = true;
		samples[sampleIndex].supportY = supportY;
		samples[sampleIndex].deltaY = deltaY;
	}

	int samplePriority[3] = { 1, 0, 2 };
	const int horizontalIntent = _getHorizontalIntent();
	if (horizontalIntent > 0) {
		samplePriority[0] = 2;
		samplePriority[1] = 1;
		samplePriority[2] = 0;
	}
	else if (horizontalIntent < 0) {
		samplePriority[0] = 0;
		samplePriority[1] = 1;
		samplePriority[2] = 2;
	}

	int bestDownwardSample = -1;
	float bestDownwardDelta = std::numeric_limits<float>::max();
	for (int i = 0; i < 3; ++i) {
		const int sampleIndex = samplePriority[i];
		if (!samples[sampleIndex].hasSupport) {
			continue;
		}

		if (samples[sampleIndex].deltaY <= 0.0f) {
			outSupportY = samples[sampleIndex].supportY;
			return true;
		}

		if (samples[sampleIndex].deltaY < bestDownwardDelta) {
			bestDownwardDelta = samples[sampleIndex].deltaY;
			bestDownwardSample = sampleIndex;
		}
	}

	if (bestDownwardSample >= 0) {
		outSupportY = samples[bestDownwardSample].supportY;
		return true;
	}

	return false;
}

Tile* Character::_findGroundSupportTile(float footY, float maxSnapDistance, float& outSupportY)
{
	Collidable* body = this->getCollidable();
	if (!body || !body->isActive() || body->getType() != COL_OBJ_SQUARE) {
		return NULL;
	}

	Square* bodySquare = (Square*)body;
	const vector2 bodyMin = bodySquare->getMin();
	const vector2 bodyMax = bodySquare->getMax();
	const float bodyWidth = bodyMax.x - bodyMin.x;
	if (bodyWidth <= 0.0f) {
		return NULL;
	}

	float sampleInset = std::min(kSupportSampleInset, bodyWidth * 0.45f);
	if (sampleInset < 0.0f) {
		sampleInset = 0.0f;
	}

	const float sampleXs[3] = {
		bodyMin.x + sampleInset,
		bodyMin.x + (bodyWidth * 0.5f),
		bodyMax.x - sampleInset
	};

	struct SupportCandidate {
		Tile* upwardTile = NULL;
		float upwardY = 0.0f;
		float upwardDistance = std::numeric_limits<float>::max();
		Tile* downwardTile = NULL;
		float downwardY = 0.0f;
		float downwardDelta = std::numeric_limits<float>::max();
	};
	SupportCandidate sampleCandidates[3];
	for (SupportCandidate& candidate : sampleCandidates) {
		candidate.upwardY = footY;
		candidate.downwardY = footY;
	}

	Game* game = Engine2D::getGame();
	if (!game || game->empty()) {
		return NULL;
	}

	ProgramState* activeProgramState = game->top();
	GameState* activeGameState = dynamic_cast<GameState*>(activeProgramState);
	if (!activeGameState) {
		return NULL;
	}

	const auto& objects = activeGameState->getObjectManager()->getObjects();
	const int horizontalIntent = _getHorizontalIntent();
	const float maxUpwardSnapDistance = std::max(maxSnapDistance, maxSnapDistance * kUpwardSnapMultiplier);
	const float maxDownwardSnapDistance = maxSnapDistance;
	const bool useUphillProbe = horizontalIntent != 0;
	const float uphillProbeX =
		(horizontalIntent > 0) ?
		(bodyMax.x + kUphillProbeDistance) :
		(bodyMin.x - kUphillProbeDistance);
	Tile* uphillProbeTile = NULL;
	float uphillProbeY = footY;
	float uphillProbeRise = std::numeric_limits<float>::max();

	for (const auto& entry : objects) {
		GameObject* object = entry.second;
		if (!object || object == this || object->getType() != GAME_OBJ_TILE) {
			continue;
		}

		Tile* tile = (Tile*)object;
		if (!tile || tile->getTileType() != "tile") {
			continue;
		}

		if (tile->isNonCollidingLayer()) {
			continue;
		}

		if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
			continue;
		}

		Collidable* tileCollidable = tile->getCollidable();
		if (!tileCollidable || !tileCollidable->isActive()) {
			continue;
		}

		for (int sampleIndex = 0; sampleIndex < 3; ++sampleIndex) {
			const float sampleX = sampleXs[sampleIndex];
			float supportY = 0.0f;
			if (!_sampleSupportY(tileCollidable, sampleX, supportY)) {
				continue;
			}

			const float deltaY = supportY - footY;
			if (deltaY < -maxUpwardSnapDistance || deltaY > maxDownwardSnapDistance) {
				continue;
			}

			// Prefer supports that resolve penetration (support at/above current footline)
			// before supports that move the character farther downward.
			if (deltaY <= 0.0f) {
				const float distance = std::fabs(deltaY);
				if (distance < sampleCandidates[sampleIndex].upwardDistance) {
					sampleCandidates[sampleIndex].upwardDistance = distance;
					sampleCandidates[sampleIndex].upwardY = supportY;
					sampleCandidates[sampleIndex].upwardTile = tile;
				}
			}
			else if (deltaY < sampleCandidates[sampleIndex].downwardDelta) {
				sampleCandidates[sampleIndex].downwardDelta = deltaY;
				sampleCandidates[sampleIndex].downwardY = supportY;
				sampleCandidates[sampleIndex].downwardTile = tile;
			}
		}

		if (!useUphillProbe || isSquareOnlyCollidable(tileCollidable)) {
			continue;
		}

		float probeSupportY = 0.0f;
		if (!_sampleSupportY(tileCollidable, uphillProbeX, probeSupportY)) {
			continue;
		}

		const float probeDeltaY = probeSupportY - footY;
		if (probeDeltaY > -kStepUpAssistEpsilon || probeDeltaY < -maxUpwardSnapDistance) {
			continue;
		}

		const float rise = std::fabs(probeDeltaY);
		if (rise > kUphillProbeMaxRise) {
			continue;
		}

		if (rise < uphillProbeRise) {
			uphillProbeRise = rise;
			uphillProbeY = probeSupportY;
			uphillProbeTile = tile;
		}
	}
	Tile* preferredTiles[3] = { NULL, NULL, NULL };
	float preferredYs[3] = { footY, footY, footY };
	bool hasPreferred[3] = { false, false, false };
	for (int sampleIndex = 0; sampleIndex < 3; ++sampleIndex) {
		SupportCandidate& candidate = sampleCandidates[sampleIndex];
		const bool hasUpwardCandidate = candidate.upwardTile != NULL;
		const bool hasDownwardCandidate = candidate.downwardTile != NULL;
		if (!hasUpwardCandidate && !hasDownwardCandidate) {
			continue;
		}

		hasPreferred[sampleIndex] = true;
		if (hasUpwardCandidate && hasDownwardCandidate) {
			const bool preferUpward = candidate.upwardDistance <= (candidate.downwardDelta + kUpwardSupportBias);
			if (preferUpward) {
				preferredTiles[sampleIndex] = candidate.upwardTile;
				preferredYs[sampleIndex] = candidate.upwardY;
			}
			else {
				preferredTiles[sampleIndex] = candidate.downwardTile;
				preferredYs[sampleIndex] = candidate.downwardY;
			}
			continue;
		}

		if (hasUpwardCandidate) {
			preferredTiles[sampleIndex] = candidate.upwardTile;
			preferredYs[sampleIndex] = candidate.upwardY;
		}
		else {
			preferredTiles[sampleIndex] = candidate.downwardTile;
			preferredYs[sampleIndex] = candidate.downwardY;
		}
	}

	int samplePriority[3] = { 1, 0, 2 };
	if (horizontalIntent > 0) {
		samplePriority[0] = 2;
		samplePriority[1] = 1;
		samplePriority[2] = 0;

		// When descending to the right, prefer center support to avoid embedding into slopes.
		if (hasPreferred[1] && hasPreferred[2] && preferredYs[2] > (preferredYs[1] + kSlopePriorityEpsilon)) {
			samplePriority[0] = 1;
			samplePriority[1] = 2;
			samplePriority[2] = 0;
		}
	}
	else if (horizontalIntent < 0) {
		samplePriority[0] = 0;
		samplePriority[1] = 1;
		samplePriority[2] = 2;

		// Symmetric downhill behavior for leftward movement.
		if (hasPreferred[1] && hasPreferred[0] && preferredYs[0] > (preferredYs[1] + kSlopePriorityEpsilon)) {
			samplePriority[0] = 1;
			samplePriority[1] = 0;
			samplePriority[2] = 2;
		}
	}

	if (uphillProbeTile) {
		outSupportY = uphillProbeY;
		return uphillProbeTile;
	}

	for (int i = 0; i < 3; ++i) {
		const int sampleIndex = samplePriority[i];
		if (!hasPreferred[sampleIndex] || !preferredTiles[sampleIndex]) {
			continue;
		}

		outSupportY = preferredYs[sampleIndex];
		return preferredTiles[sampleIndex];
	}

	return NULL;
}

void Character::_initStates() {
	std::string animationsFilePath = BasePath("Character/Animations.json");
	std::vector<Animation*> loadedAnimations;
	bool animationsFromJson = false;

	if (FileSystem::FileExists(animationsFilePath)) {
		loadedAnimations = Animations::fromJSON(animationsFilePath.c_str(), nullptr);
		if (!loadedAnimations.empty()) {
			for (Animation* animation : loadedAnimations) {
				if (animation) {
					_animationManager.store(animation);
				}
			}
		}
		animationsFromJson = !loadedAnimations.empty();
	}

	auto findLoadedAnimation = [&](const char* name) -> Animation* {
		for (Animation* animation : loadedAnimations) {
			if (animation && std::strcmp(animation->getName(), name) == 0) {
				return animation;
			}
		}
		return nullptr;
	};

	auto getAnimation = [&](const char* name, bool& loaded) -> Animation* {
		Animation* animation = animationsFromJson ? findLoadedAnimation(name) : nullptr;
		loaded = (animation != nullptr);
		if (!animation) {
			animation = _animationManager.create();
		}
		return animation;
	};

	GameObjectState* idle = this->addState("Idle");

	bool idleLoaded = false;
	Animation* idleAnimation = getAnimation("Idle", idleLoaded);

	vector2 idleFrameDimensions{ 64, 80 };

	if (!idleLoaded) {
		Texture* idleSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Idle/Idle-Sheet.png").c_str());
		Animations::createFramesForAnimation(idleAnimation, idleSheet, idleFrameDimensions, _spriteManager);
		idleAnimation->setMode(Animation::Mode::eOscillate);
		idleAnimation->setFrameRate(30);
	}

	idleAnimation->setName(idle->getName());
	idleAnimation->center();

	idle->setPreserveScaling(true);
	idle->setRenderable(idleAnimation);

	static Square idleHitBox({ -10, kLocomotionFootLocalY - 48.0f }, 20, 48);
	for (unsigned int i = 0; i < idleAnimation->getFrameCount(); i++) {
		(*idleAnimation)[i]->setCollidable(&idleHitBox);
	}

	/////////////////////////////////////////
	GameObjectState* rising = this->addState("Rising");
	rising->setPreserveScaling(true);
	rising->setExecuteTime(0.1);
	//rising->setDirection(vector2(0.0f, -1.0f));
	//rising->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.1));

	bool risingLoaded = false;
	Animation* risingAnimation = getAnimation("Rising", risingLoaded);
	vector2 risingDimensions{ 64, 64 };
	if (!risingLoaded) {
		Texture* risingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-Start/Jump-Start-Sheet.png").c_str());
		Animations::createFramesForAnimation(risingAnimation, risingSheet, risingDimensions, _spriteManager);
		risingAnimation->setFrameRate(30);
	}

	risingAnimation->setName(rising->getName());
	risingAnimation->setOffset({ 0.0, -8.0 });
	risingAnimation->center();

	rising->setRenderable(risingAnimation);

	static Square risingHitBox({ -14, kLocomotionFootLocalY - 52.0f }, 20, 52);
	for (unsigned int i = 0; i < risingAnimation->getFrameCount(); i++) {
		(*risingAnimation)[i]->setCollidable(&risingHitBox);
	}

	/////////////////////////////////////////

	GameObjectState* jump = this->addState("Jump");
	jump->setPreserveScaling(true);
	jump->setExecuteTime(0.25);
	jump->setDirection(vector2(0.0f, -1.0f));
	jump->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.67));

	bool jumpLoaded = false;
	Animation* jumpAnimation = getAnimation("Jump", jumpLoaded);

	vector2 jumpDimensions{ 64, 64 };

	if (!jumpLoaded) {
		Texture* jumpSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jumlp-All/Jump-All-Sheet.png").c_str());
		Animations::createFramesForAnimation(jumpAnimation, jumpSheet, jumpDimensions, _spriteManager, 4, 8);
		jumpAnimation->setMode(Animation::Mode::eOscillate);
		jumpAnimation->setFrameRate(60);
	}

	jumpAnimation->setName(jump->getName());
	jumpAnimation->setOffset({ 0.0, -8.0 });
	jumpAnimation->center();

	jump->setRenderable(jumpAnimation);

	static Square jumpHitBox({ -14, kLocomotionFootLocalY - 52.0f }, 20, 52);
	for (unsigned int i = 0; i < jumpAnimation->getFrameCount(); i++) {
		(*jumpAnimation)[i]->setCollidable(&jumpHitBox);
	}

	//GameObjectState* jumpLeft = this->addState("JumpLeft");
	//GameObjectState* jumpRight = this->addState("JumpRight");

	/////////////////////////////////////////

	GameObjectState* falling = this->addState("Falling");
	falling->setPreserveScaling(true);
	falling->setDirection(vector2(0.0f, 1.0f));
	falling->setForce((MOVE_UNITS * JUMP_MULTIPLIER) * 0.5);

	falling->setRenderable(jumpAnimation);

	/////////////////////////////////////////

	GameObjectState* landing = this->addState("Landing");
	landing->setDirection(vector2(0.0f, 1.0f));
	landing->setPreserveScaling(true);

	vector2 landingDimensions{ 64, 64 };

	bool landingLoaded = false;
	Animation* landingAnimation = getAnimation("Landing", landingLoaded);
	landingAnimation->setName(landing->getName());
	landingAnimation->setOffset({ 0.0, -8.0 });
	landing->setRenderable(landingAnimation);

	if (!landingLoaded) {
		Texture* landingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-End/Jump-End-Sheet.png").c_str());
		Animations::createFramesForAnimation(landingAnimation, landingSheet, landingDimensions, _spriteManager);
		landingAnimation->setFrameRate(30);
		landingAnimation->setSpeed(2.7f);
	}

	for (unsigned int i = 0; i < landingAnimation->getFrameCount(); i++) {
		(*landingAnimation)[i]->setCollidable(&jumpHitBox);
	}
	landingAnimation->center();

	/////////////////////////////////////////

	GameObjectState* runningLeft = this->addState("RunningLeft");
	GameObjectState* runningRight = this->addState("RunningRight");

	vector2 runningDimensions{ 80.0, 80.0 };

	runningLeft->setDirection({ -1.0, 0.0 });
	runningRight->setDirection({ 1.0, 0.0 });

	runningLeft->setForce(0.0f);
	runningRight->setForce(0.0f);

	bool runningLeftLoaded = false;
	bool runningRightLoaded = false;
	Animation* runningLeftAnimation = getAnimation("RunningLeft", runningLeftLoaded);
	Animation* runningRightAnimation = getAnimation("RunningRight", runningRightLoaded);

	runningLeftAnimation->setName(runningLeft->getName());
	runningRightAnimation->setName(runningRight->getName());

	runningLeft->setRenderable(runningLeftAnimation);
	runningRight->setRenderable(runningRightAnimation);

	if (!runningLeftLoaded || !runningRightLoaded) {
		Texture* runningSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Run/Run-Sheet.png").c_str());
		if (!runningLeftLoaded) {
			Animations::createFramesForAnimation(runningLeftAnimation, runningSheet, runningDimensions, _spriteManager);
		}
		if (!runningRightLoaded) {
			Animations::createFramesForAnimation(runningRightAnimation, runningSheet, runningDimensions, _spriteManager);
		}
	}

	static Square runHitBox({ -10, kLocomotionFootLocalY - 42.0f }, 26, 42);
	for (unsigned int i = 0; i < runningRightAnimation->getFrameCount(); i++) {
		(*runningLeftAnimation)[i]->setCollidable(&runHitBox);
		(*runningRightAnimation)[i]->setCollidable(&runHitBox);
	}

	runningLeftAnimation->mirror(true, false);
	runningLeftAnimation->center();

	runningRightAnimation->center();
	if (!runningLeftLoaded) {
		runningLeftAnimation->setMode(Animation::Mode::eLoop);
		runningLeftAnimation->setFrameRate(60);
		runningLeftAnimation->setSpeed(1.1f);
	}
	if (!runningRightLoaded) {
		runningRightAnimation->setMode(Animation::Mode::eLoop);
		runningRightAnimation->setFrameRate(60);
		runningRightAnimation->setSpeed(1.1f);
	}

	/////////////////////////////////////////

	GameObjectState* attack01 = this->addState("Attack01");
	GameObjectState* attack02 = this->addState("Attack02");

	attack01->setPreserveScaling(true);
	attack02->setPreserveScaling(true);

	vector2 attackDimensions{ 96.0, 80.0 };

	Texture* attackSheet = nullptr;
	bool attack01Loaded = false;
	bool attack02Loaded = false;
	Animation* attack01Animation = getAnimation("Attack01", attack01Loaded);
	if (!attack01Loaded || !attack02Loaded) {
		attackSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Attack-01/Attack-01-Sheet.png").c_str());
	}
	if (!attack01Loaded) {
		Animations::createFramesForAnimation(attack01Animation, attackSheet, attackDimensions, _spriteManager, 0, 5);
		attack01Animation->setFrameRate(60);
	}
	attack01Animation->setName(attack01->getName());
	attack01Animation->center();
	attack01->setRenderable(attack01Animation);

	Animation* attack02Animation = getAnimation("Attack02", attack02Loaded);
	if (!attack02Loaded) {
		Animations::createFramesForAnimation(attack02Animation, attackSheet, attackDimensions, _spriteManager, 5, 3);
		attack02Animation->setFrameRate(30);
	}
	attack02Animation->setName(attack02->getName());
	attack02Animation->center();
	attack02->setRenderable(attack02Animation);

	/////////////////////////////////////////

	GameObjectState* dead = this->addState("Dead");
	dead->setPreserveScaling(true);

	vector2 deadDimensions{ 80, 64 };

	Texture* deadSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Dead/Dead-Sheet.png").c_str());

	bool deadLoaded = false;
	Animation* deadAnimation = getAnimation("Dead", deadLoaded);
	if (!deadLoaded) {
		Animations::createFramesForAnimation(deadAnimation, deadSheet, deadDimensions, _spriteManager);
		deadAnimation->setFrameRate(30);
	}

	deadAnimation->setName(dead->getName());
	deadAnimation->setOffset({ 8.0, 8.0 });
	deadAnimation->center();

	dead->setRenderable(deadAnimation);

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision) {
		struct FootlineCheck {
			const char* stateName;
			GameObjectState* state;
		};

		const FootlineCheck checks[] = {
			{ "Idle", idle },
			{ "Rising", rising },
			{ "Jump", jump },
			{ "Landing", landing },
			{ "RunningLeft", runningLeft }
		};

		char buffer[256];
		for (const FootlineCheck& check : checks) {
			float footLocalY = 0.0f;
			if (!_getStateFootLocalY(check.state, footLocalY)) {
				sprintf_s(buffer, sizeof(buffer), "Character Footline [%s]: unavailable\n", check.stateName);
				DEBUG_MSG(buffer);
				continue;
			}

			sprintf_s(buffer, sizeof(buffer), "Character Footline [%s]: %.2f (target %.2f)\n",
				check.stateName, footLocalY, kLocomotionFootLocalY);
			DEBUG_MSG(buffer);

			if (std::fabs(footLocalY - kLocomotionFootLocalY) > kFootlineTolerance) {
				sprintf_s(buffer, sizeof(buffer),
					"WARNING Character Footline mismatch [%s]: %.2f vs %.2f\n",
					check.stateName, footLocalY, kLocomotionFootLocalY);
				DEBUG_MSG(buffer);
			}
		}
	}
#endif

	// This isn't really used for much at the moment...
	// But I'd like to eventually store the collision data from each of the frames
	if (!animationsFromJson) {
		std::vector<Animation*> animations;
		for (auto& animation : _animationManager) {
			if (animation) {
				animations.push_back(animation);
			}
		}
		Animations::toJSON(animations, animationsFilePath.c_str());
	}
	/////////////////////////////////////////////////////////////////////////////
}

void Character::_initTransitions() {

	std::string transitionsFilePath = BasePath("Character/Transitions.json");
	if (FileSystem::FileExists(transitionsFilePath))
	{
		FileStream fileStream = FileSystem::File::Open(transitionsFilePath);
		this->fromJSON(fileStream);
		fileStream.close();
	}
	else
	{
		// If running, and you press jump, return to running instead of idle
		// running, press jump, jumping, if previous state running, on end, return to running
		/////////////////////////////////////////
		registerTransition("Dead", "DEATH", "Idle");

		registerTransition("Idle", "JUMP_PRESSED", "Rising");
		registerTransition("Idle", "LEFT_DOWN", "RunningLeft");
		registerTransition("Idle", "RIGHT_DOWN", "RunningRight");
		registerTransition("Idle", "ATTACK_PRESSED", "Attack01");
		registerTransition("Idle", "IN_AIR", "Falling");
		registerTransition("Idle", "DEATH", "Dead");

		registerTransition("Rising", "JUMP_UP", "Falling"); // stop rising when you let go of the button
		registerTransition("Rising", "JUMP_RELEASED", "Falling");
		registerTransition("Rising", EVT_STATE_END, "Jump");

		registerTransition("Jump", "JUMP_UP", "Falling");
		registerTransition("Jump", "JUMP_RELEASED", "Falling");
		registerTransition("Jump", EVT_STATE_END, "Falling");
		registerTransition("Jump", "DEATH", "Dead");

		registerTransition("Falling", "GROUND_COLLISION", "Landing");
		registerTransition("Falling", "DEATH", "Dead");
		registerTransition("Landing", EVT_STATE_END, "Idle");
		registerTransition("Landing", "DEATH", "Dead");

		registerTransition("RunningLeft", "LEFT_RELEASED", "Idle");
		registerTransition("RunningLeft", "RIGHT_PRESSED", "RunningRight");
		registerTransition("RunningLeft", "JUMP_PRESSED", "Rising");
		registerTransition("RunningLeft", "ATTACK_PRESSED", "Attack01");
		registerTransition("RunningLeft", "IN_AIR", "Falling");
		registerTransition("RunningLeft", "DEATH", "Dead");

		registerTransition("RunningRight", "RIGHT_RELEASED", "Idle");
		registerTransition("RunningRight", "LEFT_PRESSED", "RunningLeft");
		registerTransition("RunningRight", "JUMP_PRESSED", "Rising");
		registerTransition("RunningRight", "ATTACK_PRESSED", "Attack01");
		registerTransition("RunningRight", "IN_AIR", "Falling");
		registerTransition("RunningRight", "DEATH", "Dead");

		registerTransition("Attack01", EVT_STATE_END, "Idle");
		registerTransition("Attack01", "ATTACK_PRESSED", "Attack02");
		registerTransition("Attack01", "DEATH", "Dead");
		registerTransition("Attack02", EVT_STATE_END, "Idle");
		registerTransition("Attack02", "DEATH", "Dead");

		this->toJSON(transitionsFilePath);
	}
}

void Character::onStateDidEnter(State* previous, State* current)
{
	GameObject::onStateDidEnter(previous, current);
	_pendingTransitionFootCorrection = 0.0f;

	GameObjectState* previousState = (GameObjectState*)previous;
	GameObjectState* currentState = (GameObjectState*)current;
	if (!previousState || !currentState) {
		return;
	}

	const char* currentStateName = currentState->getName();
	vector2 currentVelocity = this->getVelocity();
	bool velocityAdjusted = false;

	const bool lockHorizontalVelocity =
		!strcmp(currentStateName, "Attack01") ||
		!strcmp(currentStateName, "Attack02") ||
		!strcmp(currentStateName, "Dead");
	if (lockHorizontalVelocity && std::fabs(currentVelocity.x) > kHorizontalVelocityEpsilon) {
		currentVelocity.x = 0.0f;
		velocityAdjusted = true;
	}

	const bool resetDownwardVelocityForJumpStart =
		!strcmp(currentStateName, "Rising") ||
		!strcmp(currentStateName, "Jump");
	if (resetDownwardVelocityForJumpStart && currentVelocity.y > 0.0f) {
		currentVelocity.y = 0.0f;
		velocityAdjusted = true;
	}

	if (velocityAdjusted) {
		this->setVelocity(currentVelocity);
	}

	const bool enteringAerialState =
		!strcmp(currentStateName, "Rising") ||
		!strcmp(currentStateName, "Jump") ||
		!strcmp(currentStateName, "Falling");
	if (!enteringAerialState) {
		_longJumpMomentumActive = false;
		_longJumpMomentumDirection = 0;
		_longJumpMomentumSpeed = 0.0f;
	}
	else {
		const char* previousStateName = previousState->getName();
		const bool launchedFromRunState =
			previousStateName &&
			(!strcmp(previousStateName, "RunningLeft") || !strcmp(previousStateName, "RunningRight"));
		const float horizontalLaunchSpeed = std::fabs(currentVelocity.x);
		const int launchDirection =
			(currentVelocity.x > kHorizontalVelocityEpsilon) ? 1 :
			((currentVelocity.x < -kHorizontalVelocityEpsilon) ? -1 : 0);
		if (launchedFromRunState &&
			_isRunRequested() &&
			launchDirection != 0 &&
			horizontalLaunchSpeed >= kLongJumpLaunchSpeedThreshold) {
			_longJumpMomentumActive = true;
			_longJumpMomentumDirection = launchDirection;
			_longJumpMomentumSpeed = horizontalLaunchSpeed;
		}
	}

	float previousFootLocalY = 0.0f;
	float currentFootLocalY = 0.0f;
	if (!_getStateFootLocalY(previousState, previousFootLocalY) ||
		!_getStateFootLocalY(currentState, currentFootLocalY)) {
		return;
	}

	const float deltaY = previousFootLocalY - currentFootLocalY;
	if (std::fabs(deltaY) <= kFootlineEpsilon) {
		return;
	}

	this->setPosition(this->getPosition().x, this->getPosition().y + deltaY);
	_pendingTransitionFootCorrection = std::fabs(deltaY);
}

void Character::handleCollisionContact(const CollisionContact& contact)
{
	if (!contact.other || contact.other->getType() != GAME_OBJ_TILE) {
		return;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile) {
		return;
	}

	if (contact.phase == CollisionPhase::Exit) {
		_groundContacts.erase(tile);
		_refreshGroundTile();
		return;
	}

	if (_isGroundContact(contact)) {
		_groundContacts.insert(tile);
		_timeWithoutGroundContact = 0.0f;
		_refreshGroundTile();
	}

	if (contact.overlapping &&
		tile->getTileType() == "tile" &&
		!tile->isNonCollidingLayer() &&
		(!_isOneWayTile(tile) || _canCollideWithOneWayTile(tile)) &&
		contact.normal.has_value()) {
		const bool squareOnlySideContact = isSquareOnlyCollidable(contact.otherCollidable);
		const int horizontalIntent = _getHorizontalInput();
		if (horizontalIntent != 0) {
			const vector2 normal = contact.normal.value();
			const float absNormalX = std::fabs(normal.x);
			if (absNormalX > kWallNormalThreshold && absNormalX > std::fabs(normal.y)) {
				const bool pushingIntoWall =
					(horizontalIntent > 0 && normal.x > 0.0f) ||
					(horizontalIntent < 0 && normal.x < 0.0f);
				if (pushingIntoWall) {
					float maxStepUpDistance = kDefaultMaxSnapPerFrame;
					Tile* stepContextTile = tile ? tile : _tile;
					if (stepContextTile && stepContextTile->getTileSet()) {
						maxStepUpDistance = std::max(1.0f, stepContextTile->getTileSet()->getTileSize() * 0.5f);
					}

					float footY = this->getPosition().y;
					if (Collidable* bodyCollidable = this->getCollidable()) {
						if (bodyCollidable->getType() == COL_OBJ_SQUARE) {
							Square* bodySquare = (Square*)bodyCollidable;
							footY = bodySquare->getMax().y;
						}
					}

					bool isGroundedForStepAssist = false;
					if (_timeWithoutGroundContact < kGroundLossGraceSeconds) {
						if (GameObjectState* currentState = this->getState()) {
							const char* stateName = currentState->getName();
							isGroundedForStepAssist =
								_isGroundedLocomotionState(stateName) ||
								!strcmp(stateName, "Attack01") ||
								!strcmp(stateName, "Attack02");
						}
					}

					if (isGroundedForStepAssist) {
						float stepSupportY = footY;
						Tile* stepSupportTile = _findGroundSupportTile(footY, maxStepUpDistance, stepSupportY);
						const float stepDeltaY = stepSupportY - footY;
						if (stepSupportTile && stepDeltaY < -kStepUpAssistEpsilon) {
							// Help transition from flat square tops to uphill supports (slopes/stairs).
							this->setPosition(
								this->getPosition().x,
								this->getPosition().y + std::max(-maxStepUpDistance, stepDeltaY));
							return;
						}
					}

					if (squareOnlySideContact) {
						float separationX = kHorizontalSeparationEpsilon;
						if (contact.penetrationDepth.has_value() && contact.penetrationDepth.value() > 0.0f) {
							separationX += contact.penetrationDepth.value() / std::max(absNormalX, 0.001f);
						}
						separationX = std::min(separationX, kMaxHorizontalSeparationPerContact);

						// Keep tile side contacts from letting horizontal motion push inside walls.
						this->setPosition(
							this->getPosition().x - std::copysign(separationX, normal.x),
							this->getPosition().y);
					}
				}
			}
		}
	}

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision && Debug::dbgTiles)
	{
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Tile (%i):\n\tpos{ % f,% f }\n", tile->getTileIndex(), tile->_position.x, tile->_position.y);
		DEBUG_MSG(buffer);

		if (Renderable* renderable = tile->getRenderable()) {
			sprintf_s(buffer, sizeof(buffer), "\trenderablePos{%f, %f}\n", renderable->getPosition().x, renderable->getPosition().y);
			DEBUG_MSG(buffer);
		}

		if (Collidable* collidable = tile->getCollidable()) {
			switch (collidable->getType()) {
			case COL_OBJ_SQUARE: {
				Square* square = (Square*)contact.other->getCollidable();
				sprintf_s(buffer, sizeof(buffer), "\tcolSquare{%f, %f, %f, %f}\n", square->_x, square->_y, square->getMax().x, square->getMax().y);
				DEBUG_MSG(buffer);
				break;
			}
			case COL_OBJ_POLYGON:
				DEBUG_MSG("\tcolPolygon\n");
				break;
			case COL_OBJ_VOID:
				break;
			case COL_OBJ_CIRCLE:
				break;
			case COL_OBJ_PLANE:
				break;
			case COL_OBJ_GROUP:
				break;
			}
		}
	}
#endif

	if (tile->getTileType() == "key" && contact.phase == CollisionPhase::Enter)
	{
		if (Renderable* renderable = tile->getRenderable()) {
			renderable->setVisibility(false);
		}
	}
}

const char* Character::mapCollisionToCommand(const CollisionContact& contact) const
{
	if (!contact.other || contact.phase == CollisionPhase::Exit || contact.other->getType() != GAME_OBJ_TILE) {
		return NULL;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile) {
		return NULL;
	}

	if (tile->getTileType() == "key") {

		if (Collidable* collidable = tile->getCollidable()) {	

			if (tile->getLayerCollisionMode() != TileCollisionMode::None && contact.phase == CollisionPhase::Enter) 
			{
				tile->setLayerCollisionMode(TileCollisionMode::None);
				return "DEATH";
			}
		}
	}

	if (tile->getTileType() != "tile") {
		return NULL;
	}

	if (contact.normal) {
		if (contact.normal->y < -0.5f) {
			return "JUMP_RELEASED";
		}
		if (contact.normal->y > kGroundNormalThreshold) {
			if (contact.phase == CollisionPhase::Enter) {
				return "GROUND_COLLISION";
			}
			if (contact.phase == CollisionPhase::Stay) {
				if (GameObjectState* state = this->getState()) {
					if (!strcmp(state->getName(), "Falling")) {
						return "GROUND_COLLISION";
					}
				}
			}
		}
	}
	else {
		vector2 directionToObject = tile->getPosition() - this->getPosition();
		directionToObject.normalize();
		if (directionToObject.y < -0.5f) {
			return "JUMP_RELEASED";
		}
		if (directionToObject.y > kGroundNormalThreshold) {
			if (contact.phase == CollisionPhase::Enter) {
				return "GROUND_COLLISION";
			}
			if (contact.phase == CollisionPhase::Stay) {
				if (GameObjectState* state = this->getState()) {
					if (!strcmp(state->getName(), "Falling")) {
						return "GROUND_COLLISION";
					}
				}
			}
		}
	}

	return NULL;
}

void Character::update(float time)
{
	if (GameObjectState* preUpdateState = this->getState()) {
		const char* preUpdateStateName = preUpdateState->getName();
		const bool freezeHorizontalPreUpdate =
			!strcmp(preUpdateStateName, "Attack01") ||
			!strcmp(preUpdateStateName, "Attack02") ||
			!strcmp(preUpdateStateName, "Dead");
		if (freezeHorizontalPreUpdate) {
			vector2 preUpdateVelocity = this->getVelocity();
			if (std::fabs(preUpdateVelocity.x) > kHorizontalVelocityEpsilon) {
				preUpdateVelocity.x = 0.0f;
				this->setVelocity(preUpdateVelocity);
			}
		}
	}

	GameObject::update(time);

	if (_dropThroughTimer > 0.0f) {
		_dropThroughTimer = std::max(0.0f, _dropThroughTimer - time);
	}

	_refreshGroundTile();

	if (_isDropThroughRequested()) {
		bool groundedOnOneWay = _isOneWayTile(_tile);

		if (!groundedOnOneWay) {
			for (Tile* contactTile : _groundContacts) {
				if (_isOneWayTile(contactTile) && _canCollideWithOneWayTile(contactTile)) {
					groundedOnOneWay = true;
					break;
				}
			}
		}

		if (groundedOnOneWay) {
			_startDropThrough();
		}
	}

	float baseSnapPerFrame = kDefaultMaxSnapPerFrame;
	if (_tile && _tile->getTileSet()) {
		baseSnapPerFrame = std::max(1.0f, _tile->getTileSet()->getTileSize() * 0.5f);
	}
	float maxSnapPerFrame = baseSnapPerFrame;
	if (_pendingTransitionFootCorrection > 0.0f) {
		maxSnapPerFrame = std::max(baseSnapPerFrame, _pendingTransitionFootCorrection + 1.0f);
	}
	const float horizontalTravelPerFrame = std::fabs(this->getVelocity().x) * time;
	maxSnapPerFrame = std::max(maxSnapPerFrame, horizontalTravelPerFrame + kHorizontalSnapTravelPadding);

	float footY = this->getPosition().y;
	if (Collidable* bodyCollidable = this->getCollidable()) {
		if (bodyCollidable->getType() == COL_OBJ_SQUARE) {
			Square* bodySquare = (Square*)bodyCollidable;
			footY = bodySquare->getMax().y;
		}
	}

	float supportY = footY;
	Tile* supportTile = _findGroundSupportTile(footY, maxSnapPerFrame, supportY);
	if (supportTile && _tile && supportTile != _tile) {
		float stickySupportY = footY;
		if (_findSupportOnTile(_tile, footY, maxSnapPerFrame, stickySupportY) &&
			std::fabs(stickySupportY - supportY) <= kSupportSwitchHysteresis) {
			supportTile = _tile;
			supportY = stickySupportY;
		}
	}

	bool hasGroundSupport = (supportTile != NULL);
	if (supportTile) {
		_tile = supportTile;
	}

	GameObjectState* state = this->getState();

	if (!hasGroundSupport) {
		_timeWithoutGroundContact += time;
	}
	else {
		_timeWithoutGroundContact = 0.0f;
	}

	if (state) {
		const char* stateName = state->getName();
		if (hasGroundSupport && !strcmp(stateName, "Falling")) {
			this->sendInput("GROUND_COLLISION");
			state = this->getState();
			if (!state) {
				_pendingTransitionFootCorrection = 0.0f;
				return;
			}
			stateName = state->getName();
		}

		const bool groundedLocomotionState = _isGroundedLocomotionState(stateName);
		const bool shouldApplyGroundSnap =
			groundedLocomotionState ||
			!strcmp(stateName, "Attack01") ||
			!strcmp(stateName, "Attack02");

		if (hasGroundSupport && shouldApplyGroundSnap) {
			float targetFootY = supportY;
			if (supportTile) {
				if (Collidable* supportCollidable = supportTile->getCollidable()) {
					if (!isSquareOnlyCollidable(supportCollidable)) {
						// Keep the character slightly above sloped/polygon supports to avoid visible embedding.
						targetFootY -= kSlopeFootClearance;
					}
				}
			}

			float deltaY = targetFootY - footY;
			if (std::fabs(deltaY) > 0.001f) {
				deltaY = std::max(-maxSnapPerFrame, std::min(maxSnapPerFrame, deltaY));
				this->setPosition(this->getPosition().x, this->getPosition().y + deltaY);
				footY += deltaY;
			}
		}

		const bool canInputMove =
			!strcmp(stateName, "RunningLeft") ||
			!strcmp(stateName, "RunningRight") ||
			!strcmp(stateName, "Falling") ||
			!strcmp(stateName, "Jump") ||
			!strcmp(stateName, "Rising");
		const bool canResidualMove =
			canInputMove ||
			!strcmp(stateName, "Idle") ||
			!strcmp(stateName, "Landing");
		const bool isAerialState =
			!strcmp(stateName, "Falling") ||
			!strcmp(stateName, "Jump") ||
			!strcmp(stateName, "Rising");
		const bool groundedForMovement = hasGroundSupport || (_timeWithoutGroundContact < kGroundLossGraceSeconds);

		const int horizontalInput = canInputMove ? _getHorizontalInput() : 0;
		const bool hasDirectionalIntent = horizontalInput != 0;
		const bool runRequested = canInputMove && groundedForMovement && _isRunRequested();
		_runBoostActive = runRequested && hasDirectionalIntent && _stamina > 0.0f;

		if (_runBoostActive) {
			_stamina = std::max(0.0f, _stamina - (kStaminaDrainPerSecond * time));
		}
		else {
			_stamina = std::min(_maxStamina, _stamina + (kStaminaRegenPerSecond * time));
		}

		if (_stamina <= 0.0f) {
			_runBoostActive = false;
		}

		Player* player = Engine2D::getGame()->getPlayerWith(this);
		bool downHeld = false;
		if (player && player->getController()) {
			if (Action* downAction = player->getController()->getAction("DOWN")) {
				downHeld = downAction->isActive();
			}
		}

		float horizontalVelocity = this->getVelocity().x;
		const float startingAbsHorizontalSpeed = std::fabs(horizontalVelocity);
		if (canResidualMove) {
			const float groundedMaxSpeed = _runBoostActive ? kRunMaxHorizontalSpeed : kWalkMaxHorizontalSpeed;
			const float targetVelocityX = hasDirectionalIntent ?
				((float)horizontalInput * (groundedForMovement ? groundedMaxSpeed : kAirMaxHorizontalSpeed)) :
				0.0f;
			const bool reversingInput =
				hasDirectionalIntent && ((horizontalVelocity * (float)horizontalInput) < -kHorizontalVelocityEpsilon);

			float acceleration = 0.0f;
			if (hasDirectionalIntent) {
				if (groundedForMovement) {
					acceleration = reversingInput ?
						kGroundTurnAcceleration :
						(_runBoostActive ? kRunGroundAcceleration : kWalkGroundAcceleration);
				}
				else {
					acceleration = reversingInput ? kAirTurnAcceleration : kAirAcceleration;
				}
			}
			else {
				acceleration = groundedForMovement ? kGroundDeceleration : kAirDeceleration;
			}

			const float maxDelta = acceleration * time;
			if (horizontalVelocity < targetVelocityX) {
				horizontalVelocity = std::min(horizontalVelocity + maxDelta, targetVelocityX);
			}
			else if (horizontalVelocity > targetVelocityX) {
				horizontalVelocity = std::max(horizontalVelocity - maxDelta, targetVelocityX);
			}

			if (groundedForMovement) {
				horizontalVelocity = std::max(-groundedMaxSpeed, std::min(groundedMaxSpeed, horizontalVelocity));
			}
			else {
				const float airSpeedClamp = std::max(kAirMaxHorizontalSpeed, startingAbsHorizontalSpeed);
				horizontalVelocity = std::max(-airSpeedClamp, std::min(airSpeedClamp, horizontalVelocity));
			}

			if (!hasDirectionalIntent && std::fabs(horizontalVelocity) < kHorizontalVelocityEpsilon) {
				horizontalVelocity = 0.0f;
			}
		}
		else {
			horizontalVelocity = 0.0f;
			_runBoostActive = false;
		}

		if (!groundedForMovement && isAerialState && _longJumpMomentumActive) {
			const bool oppositeDirectionInput =
				hasDirectionalIntent && (horizontalInput != _longJumpMomentumDirection);
			const bool wrongDirectionVelocity =
				(horizontalVelocity * (float)_longJumpMomentumDirection) < -kHorizontalVelocityEpsilon;

			if (oppositeDirectionInput || wrongDirectionVelocity || _longJumpMomentumDirection == 0) {
				_longJumpMomentumActive = false;
				_longJumpMomentumDirection = 0;
				_longJumpMomentumSpeed = 0.0f;
			}
			else {
				_longJumpMomentumSpeed = std::max(0.0f, _longJumpMomentumSpeed - (kLongJumpMomentumDecayPerSecond * time));
				const float signedMomentumSpeed = (float)_longJumpMomentumDirection * _longJumpMomentumSpeed;
				const float signedHorizontalSpeed = horizontalVelocity * (float)_longJumpMomentumDirection;
				if (signedHorizontalSpeed < _longJumpMomentumSpeed) {
					horizontalVelocity = signedMomentumSpeed;
				}

				if (_longJumpMomentumSpeed <= kWalkMaxHorizontalSpeed) {
					_longJumpMomentumActive = false;
					_longJumpMomentumDirection = 0;
					_longJumpMomentumSpeed = 0.0f;
				}
			}
		}
		else if (groundedForMovement && _longJumpMomentumActive) {
			_longJumpMomentumActive = false;
			_longJumpMomentumDirection = 0;
			_longJumpMomentumSpeed = 0.0f;
		}

		vector2 velocity = this->getVelocity();
		velocity.x = horizontalVelocity;
		this->setVelocity(velocity);

		const bool lockDownwardVelocityOnGround =
			hasGroundSupport &&
			(groundedLocomotionState ||
			 !strcmp(stateName, "Attack01") ||
			 !strcmp(stateName, "Attack02") ||
			 !strcmp(stateName, "Dead"));
		if (lockDownwardVelocityOnGround && velocity.y > 0.0f) {
			velocity.y = 0.0f;
			this->setVelocity(velocity);
		}

		if (!groundedForMovement && isAerialState && downHeld && velocity.y > 0.0f) {
			velocity.y = std::min(kFastFallMaxSpeed, velocity.y + (kFastFallAcceleration * time));
			this->setVelocity(velocity);
		}

		if (canInputMove && hasDirectionalIntent && this->getRenderable()) {
			vector2 scale = this->getRenderable()->getScale();
			const float absScaleX = std::fabs(scale.x);
			this->getRenderable()->setScale((horizontalInput > 0) ? absScaleX : -absScaleX, scale.y);
		}

		if ((!strcmp(stateName, "RunningLeft") || !strcmp(stateName, "RunningRight")) &&
			state->getRenderable() &&
			state->getRenderable()->getRenderableType() == RENDERABLE_TYPE_ANIMATION) {
			Animation* runAnimation = (Animation*)state->getRenderable();
			const float targetAnimationSpeed = _runBoostActive ? kRunBoostAnimationSpeed : kRunAnimationSpeed;
			if (std::fabs(runAnimation->getSpeed() - targetAnimationSpeed) > 0.001f) {
				runAnimation->setSpeed(targetAnimationSpeed);
			}
		}

		if (player) {
//#if _DEBUG
			if (KEYBOARD) {
				if (Engine2D::getInput()->getKeyboard()->keyPressed(KEYBOARD->getKeys().KBK_F)) {
					this->sendInput("DEATH");
				}
			}
//#endif
			// Having this idea about conditional state changes,
			// like having a "KEEP_ALIVE" condition in the event queue, and in the lack of that condition,
			// the character will fall or otherwise change state

			// Grounded state is tracked by collision Enter/Stay/Exit callbacks.
			// Debounce loss slightly to avoid one-frame Enter/Exit jitter.
			if (!hasGroundSupport && _timeWithoutGroundContact >= kGroundLossGraceSeconds) {
				if (_isGroundedLocomotionState(stateName)) {
					this->sendInput("IN_AIR");
				}
			}
		} // if (Player)
	}
	_pendingTransitionFootCorrection = 0.0f;
#if _DEBUG
	if (DEBUGGING && Debug::dbgObjects)
	{
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Character:\n\tpos{ % f,% f }\n", this->_position.x, this->_position.y);
		DEBUG_MSG(buffer);

		if (Renderable* renderable = this->getRenderable()) {
			sprintf_s(buffer, sizeof(buffer), "\trenderablePos{%f, %f}\n", renderable->getPosition().x, renderable->getPosition().y);
			DEBUG_MSG(buffer);
		}

		if (Collidable* collidable = this->getCollidable()) {
			switch (collidable->getType()) {
			case COL_OBJ_SQUARE: {
				Square* square = (Square*)this->getCollidable();
				sprintf_s(buffer, sizeof(buffer), "\tcolSquare{%f, %f, %f, %f}\n", square->_x, square->_y, square->getMax().x, square->getMax().y);
				DEBUG_MSG(buffer);
				break;
			}
			case COL_OBJ_POLYGON:
				DEBUG_MSG("\tcolPolygon\n");
				break;
			case COL_OBJ_VOID:
				break;
			case COL_OBJ_CIRCLE:
				break;
			case COL_OBJ_PLANE:
				break;
			case COL_OBJ_GROUP:
				break;
			}
		}
	}
#endif
}
