// CharacterMovement.cpp

#include "Character.h"
#include "CharacterTuning.h"

#include "../CollidableGroup.h"
#include "../GameState.h"
#include "../Kinematics2D.h"
#include "../Polygon.h"
#include "../Square.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>

bool Character::_isGroundContact(const CollisionContact& contact) const
{
	if (!contact.other || contact.phase == CollisionPhase::Exit || contact.other->getType() != GAME_OBJ_TILE) {
		return false;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile) {
		return false;
	}

	if (tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
		return false;
	}

	// Reject near-vertical wall contacts as ground before running support sampling.
	// Without this guard, jumping into a wall while holding horizontal input can
	// keep ground-contact grace alive and effectively "stick" the character mid-air.
	if (contact.normal.has_value()) {
		const vector2 normal = contact.normal.value();
		const float absNormalX = std::fabs(normal.x);
		const float absNormalY = std::fabs(normal.y);
		if (absNormalX >= kGroundRejectWallNormalX && absNormalY <= kGroundRejectWallNormalY) {
			return false;
		}
	}
	if (contact.separation.has_value()) {
		const vector2 separation = contact.separation.value();
		const float absSepX = std::fabs(separation.x);
		const float absSepY = std::fabs(separation.y);
		if (absSepX > (absSepY + kHorizontalSeparationEpsilon) && absSepX > kHorizontalSeparationEpsilon) {
			return false;
		}
	}
	return Kinematics2D::isGroundContact(
		contact,
		kGroundNormalThreshold,
		kOneWayTopApproachEpsilon,
		kGroundSupportSnapDistance);
}

bool Character::_isWallBlockingContact(const CollisionContact& contact, int horizontalIntent, float footY, float maxStepUpDistance) const
{
	if (!contact.other || contact.phase == CollisionPhase::Exit || contact.other->getType() != GAME_OBJ_TILE) {
		return false;
	}

	if (horizontalIntent == 0 || !contact.normal.has_value()) {
		return false;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile || tile->getTileType() != "tile" || tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
		return false;
	}

	if (!Kinematics2D::isWallBlockingContact(contact, horizontalIntent, kWallNormalThreshold)) {
		return false;
	}

	Collidable* tileCollidable = contact.otherCollidable ? contact.otherCollidable : tile->getCollidable();
	if (!tileCollidable || !tileCollidable->isActive()) {
		return false;
	}

	float localSupportY = footY;
	if (_findSupportOnTile(tile, footY, maxStepUpDistance, localSupportY)) {
		const float localDelta = localSupportY - footY;
		const float maxUpwardSnapDistance = std::max(maxStepUpDistance, maxStepUpDistance * kUpwardSnapMultiplier);
		if (localDelta <= maxStepUpDistance && localDelta >= -maxUpwardSnapDistance) {
			return false;
		}
	}

	// On polygon tiles, support on the contacted tile takes precedence over wall pushback.
	if (!Kinematics2D::isSquareOnly(tileCollidable)) {
		float polygonSupportY = footY;
		if (_findSupportOnTile(tile, footY, maxStepUpDistance * kUpwardSnapMultiplier, polygonSupportY)) {
			return false;
		}
	}

	vector2 tileMin(0.0f, 0.0f);
	vector2 tileMax(0.0f, 0.0f);
	if (Kinematics2D::tryGetActiveBounds(tileCollidable, tileMin, tileMax)) {
		const float footWindowBottom = footY + maxStepUpDistance;
		if (tileMin.y >= footWindowBottom) {
			return false;
		}
	}

	return true;
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

bool Character::_canTriggerGroundCollisionFromFalling() const
{
	return _kinematic2DState().fallingLandingDebounceTimer <= 0.0f;
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
	int bestTileIndex = std::numeric_limits<int>::max();
	bool bestTileHasSupportSample = false;
	float bestSupportDeltaAbs = std::numeric_limits<float>::max();
	float bestDistance = std::numeric_limits<float>::max();
	std::vector<GameObject*> staleContacts;

	float sampleX = 0.0f;
	float bodyBottom = 0.0f;
	bool hasBodySupportSample = false;
	if (Collidable* selfCollidable = this->getCollidable()) {
		vector2 selfMin(0.0f, 0.0f);
		vector2 selfMax(0.0f, 0.0f);
		if (Kinematics2D::tryGetActiveBounds(selfCollidable, selfMin, selfMax)) {
			sampleX = selfMin.x + ((selfMax.x - selfMin.x) * 0.5f);
			bodyBottom = selfMax.y;
			hasBodySupportSample = true;
		}
	}

	for (GameObject* contactObject : _kinematic2DState().groundContacts) {
		Tile* tile = dynamic_cast<Tile*>(contactObject);
		if (!tile) {
			staleContacts.push_back(contactObject);
			continue;
		}
		const int tileIndex = tile->getTileIndex();

		if (tile->isNonCollidingLayer()) {
			staleContacts.push_back(contactObject);
			continue;
		}

		if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
			staleContacts.push_back(contactObject);
			continue;
		}

		Collidable* collidable = tile->getCollidable();
		if (!collidable || !collidable->isActive()) {
			staleContacts.push_back(contactObject);
			continue;
		}

		bool hasSupportSample = false;
		float supportDeltaAbs = std::numeric_limits<float>::max();
		if (hasBodySupportSample) {
			// Use multi-sample sweep across the character width, not just center
			const Collidable* selfCollidable = this->getCollidable();
			if (selfCollidable) {
				vector2 selfMin(0.0f, 0.0f), selfMax(0.0f, 0.0f);
				if (Kinematics2D::tryGetActiveBounds(selfCollidable, selfMin, selfMax)) {
					const float bodyWidth = selfMax.x - selfMin.x;
					const float sampleFractions[] = { 0.10f, 0.25f, 0.50f, 0.75f, 0.90f };
					const float sampleOffsets[] = { -5.0f, -3.0f, -1.0f, 0.0f, 1.0f, 3.0f, 5.0f };
					
					// Try to find support at any sampled point
					for (float frac : sampleFractions) {
						for (float off : sampleOffsets) {
							const float testX = selfMin.x + (bodyWidth * frac) + off;
							float supportY = 0.0f;
							if (_sampleSupportY(collidable, testX, supportY)) {
								const float supportDelta = bodyBottom - supportY;
								// Very loose tolerance for persistent ground retention on narrow tiles
								const bool nearTopSurface =
									supportDelta >= -100.0f &&  // Allow surface even significantly above feet
									supportDelta <= kGroundSupportSnapDistance;
								if (nearTopSurface) {
									hasSupportSample = true;
									const float deltaAbs = std::fabs(supportDelta);
									if (deltaAbs < supportDeltaAbs) {
										supportDeltaAbs = deltaAbs;
									}
									break;  // Found support at this fraction, move to next
								}
							}
						}
						if (hasSupportSample) break;  // Found support, no need to check other fractions
					}
				}
			}
		}

		vector2 delta(
			tile->getPosition().x - this->getPosition().x,
			tile->getPosition().y - this->getPosition().y);
		float distance = delta.norm();

		if (hasSupportSample) {
			if (!bestTileHasSupportSample ||
				supportDeltaAbs < bestSupportDeltaAbs ||
				(std::fabs(supportDeltaAbs - bestSupportDeltaAbs) <= kFootlineEpsilon &&
					(distance < bestDistance ||
						(std::fabs(distance - bestDistance) <= kFootlineEpsilon && tileIndex < bestTileIndex)))) {
				bestTileHasSupportSample = true;
				bestSupportDeltaAbs = supportDeltaAbs;
				bestDistance = distance;
				bestTile = tile;
				bestTileIndex = tileIndex;
			}
			continue;
		}

		if (!bestTileHasSupportSample &&
			(distance < bestDistance ||
				(std::fabs(distance - bestDistance) <= kFootlineEpsilon && tileIndex < bestTileIndex))) {
			bestDistance = distance;
			bestTile = tile;
			bestTileIndex = tileIndex;
		}
	}

	for (GameObject* staleContact : staleContacts) {
		_kinematic2DState().groundContacts.erase(staleContact);
	}

	_tile = bestTile;
}

bool Character::_sampleSupportY(const Collidable* collidable, float sampleX, float& outY) const
{
	return Kinematics2D::sampleSupportY(collidable, sampleX, outY);
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

	const float maxUpwardSnapDistance = std::min(
		kMaxAutoStepUpDistance,
		std::max(maxSnapDistance, maxSnapDistance * kUpwardSnapMultiplier));
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

Tile* Character::_findGroundSupportTile(float footY, float maxSnapDistance, float& outSupportY, int* outSupportSampleSource)
{
	outSupportY = footY;
	if (outSupportSampleSource) {
		*outSupportSampleSource = (int)AutoSupportSource::None;
	}

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
		int upwardTileIndex = std::numeric_limits<int>::max();
		float upwardY = 0.0f;
		float upwardDistance = std::numeric_limits<float>::max();
		Tile* downwardTile = NULL;
		int downwardTileIndex = std::numeric_limits<int>::max();
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
	const float maxUpwardSnapDistance = std::min(
		kMaxAutoStepUpDistance,
		std::max(maxSnapDistance, maxSnapDistance * kUpwardSnapMultiplier));
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
		const int tileIndex = tile->getTileIndex();

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
				const bool betterUpwardCandidate =
					distance < (sampleCandidates[sampleIndex].upwardDistance - kFootlineEpsilon) ||
					(std::fabs(distance - sampleCandidates[sampleIndex].upwardDistance) <= kFootlineEpsilon &&
						(tileIndex < sampleCandidates[sampleIndex].upwardTileIndex ||
							(tileIndex == sampleCandidates[sampleIndex].upwardTileIndex &&
								supportY < (sampleCandidates[sampleIndex].upwardY - kFootlineEpsilon))));
				if (betterUpwardCandidate) {
					sampleCandidates[sampleIndex].upwardDistance = distance;
					sampleCandidates[sampleIndex].upwardY = supportY;
					sampleCandidates[sampleIndex].upwardTile = tile;
					sampleCandidates[sampleIndex].upwardTileIndex = tileIndex;
				}
			}
			else {
				const bool betterDownwardCandidate =
					deltaY < (sampleCandidates[sampleIndex].downwardDelta - kFootlineEpsilon) ||
					(std::fabs(deltaY - sampleCandidates[sampleIndex].downwardDelta) <= kFootlineEpsilon &&
						(tileIndex < sampleCandidates[sampleIndex].downwardTileIndex ||
							(tileIndex == sampleCandidates[sampleIndex].downwardTileIndex &&
								supportY < (sampleCandidates[sampleIndex].downwardY - kFootlineEpsilon))));
				if (!betterDownwardCandidate) {
					continue;
				}

				sampleCandidates[sampleIndex].downwardDelta = deltaY;
				sampleCandidates[sampleIndex].downwardY = supportY;
				sampleCandidates[sampleIndex].downwardTile = tile;
				sampleCandidates[sampleIndex].downwardTileIndex = tileIndex;
			}
		}

		if (!useUphillProbe || Kinematics2D::isSquareOnly(tileCollidable)) {
			continue;
		}

		float probeSupportY = 0.0f;
		if (!_sampleSupportY(tileCollidable, uphillProbeX, probeSupportY)) {
			continue;
		}

		const float probeDeltaY = probeSupportY - footY;
		if (probeDeltaY > -kStepUpAssistEpsilon * 1.25f || probeDeltaY < -maxUpwardSnapDistance * 1.25f) {
			continue;
		}

		const float rise = std::fabs(probeDeltaY);
		if (rise > kUphillProbeMaxRise) {
			continue;
		}

		const bool betterUphillProbe =
			rise < (uphillProbeRise - kFootlineEpsilon) ||
			(std::fabs(rise - uphillProbeRise) <= kFootlineEpsilon &&
				(!uphillProbeTile || tileIndex < uphillProbeTile->getTileIndex()));
		if (betterUphillProbe) {
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

	// Bias: if both edge and center are available and center is close to edge,
	// prefer center to reduce oscillation on polygon slopes.
	if (hasPreferred[1]) {
		for (int edge : {0, 2}) {
			if (hasPreferred[edge] && std::fabs(preferredYs[edge] - preferredYs[1]) <= kSlopePriorityEpsilon) {
				samplePriority[0] = 1;
				samplePriority[1] = edge;
				samplePriority[2] = (edge == 0 ? 2 : 0);
				break;
			}
		}
	}

	if (uphillProbeTile) {
		outSupportY = uphillProbeY;
		if (outSupportSampleSource) {
			*outSupportSampleSource = (int)AutoSupportSource::UphillProbe;
		}
		return uphillProbeTile;
	}

	for (int i = 0; i < 3; ++i) {
		const int sampleIndex = samplePriority[i];
		if (!hasPreferred[sampleIndex] || !preferredTiles[sampleIndex]) {
			continue;
		}

		outSupportY = preferredYs[sampleIndex];
		if (outSupportSampleSource) {
			switch (sampleIndex) {
			case 0:
				*outSupportSampleSource = (int)AutoSupportSource::Left;
				break;
			case 1:
				*outSupportSampleSource = (int)AutoSupportSource::Center;
				break;
			case 2:
				*outSupportSampleSource = (int)AutoSupportSource::Right;
				break;
			default:
				*outSupportSampleSource = (int)AutoSupportSource::None;
				break;
			}
		}
		return preferredTiles[sampleIndex];
	}

	return NULL;
}
