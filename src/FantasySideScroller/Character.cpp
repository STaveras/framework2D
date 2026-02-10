// Character.cpp

#include "Character.h"

#include "../Animation.h"
#include "../CollidableGroup.h"
#include "../Polygon.h"
#include "../Square.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr float kGroundLossGraceSeconds = 0.06f;
constexpr float kDropThroughDurationSeconds = 0.20f;
constexpr float kGroundNormalThreshold = 0.2f;
constexpr float kOneWayTopApproachEpsilon = 1.0f;
// Heuristic: larger snap distance to tolerate steeper polygon slopes and
// minor sampling misalignment. Keep conservative but allow more leeway.
constexpr float kGroundSupportSnapDistance = 50.0f;
constexpr float kLocomotionFootLocalY = 24.0f;
constexpr float kFootlineTolerance = 0.5f;
constexpr float kFootlineEpsilon = 0.001f;
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
constexpr float kFastFallAcceleration = 900.0f;
constexpr float kFastFallMaxSpeed = 300.0f;
constexpr float kGravityAcceleration = 760.0f;
constexpr float kNormalFallMaxSpeed = 260.0f;
constexpr float kLongJumpLaunchSpeedThreshold = 110.0f;
constexpr float kLongJumpMomentumDecayPerSecond = 45.0f;

bool tryGetCollidableBounds(const Collidable* collidable, vector2& outMin, vector2& outMax)
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	switch (collidable->getType()) {
	case COL_OBJ_SQUARE: {
		const Square* square = (const Square*)collidable;
		if (!square) {
			return false;
		}
		outMin = square->getMin();
		outMax = square->getMax();
		return true;
	}
	case COL_OBJ_POLYGON: {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (!polygon || !polygon->isValid()) {
			return false;
		}
		outMin = polygon->getMin();
		outMax = polygon->getMax();
		return true;
	}
	case COL_OBJ_GROUP: {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group || group->empty()) {
			return false;
		}

		bool foundAny = false;
		vector2 minBounds(0.0f, 0.0f);
		vector2 maxBounds(0.0f, 0.0f);
		for (const Collidable* member : *group) {
			vector2 memberMin(0.0f, 0.0f);
			vector2 memberMax(0.0f, 0.0f);
			if (!member || !tryGetCollidableBounds(member, memberMin, memberMax)) {
				continue;
			}

			if (!foundAny) {
				minBounds = memberMin;
				maxBounds = memberMax;
				foundAny = true;
				continue;
			}

			minBounds.x = std::min(minBounds.x, memberMin.x);
			minBounds.y = std::min(minBounds.y, memberMin.y);
			maxBounds.x = std::max(maxBounds.x, memberMax.x);
			maxBounds.y = std::max(maxBounds.y, memberMax.y);
		}

		if (!foundAny) {
			return false;
		}

		outMin = minBounds;
		outMax = maxBounds;
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

void Character::resetForRespawn(void)
{
	_tile = NULL;
	_groundContacts.clear();
	_timeWithoutGroundContact = 0.0f;
	_pendingTransitionFootCorrection = 0.0f;
	_dropThroughTimer = 0.0f;
	_runBoostActive = false;
	_longJumpMomentumActive = false;
	_longJumpMomentumDirection = 0;
	_longJumpMomentumSpeed = 0.0f;
	this->setVelocity(vector2(0.0f, 0.0f));
}

bool Character::_isOneWayTile(const Tile* tile) const
{
	return tile &&
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
    // During attack states the character is invincible to non‑terrain collisions.
    // We still need a collidable to remain grounded, but we should ignore
    // interactions with enemies or hazards while attacking.  To accomplish this
    // we early out here and only allow collisions with tiles when the current
    // state is an attack.
    if (const GameObjectState* state = this->getState()) {
        const char* stateName = state->getName();
        if (stateName && (!std::strcmp(stateName, "Attack01") || !std::strcmp(stateName, "Attack02"))) {
            // Only collide with tiles while attacking
            if (other.getType() != GAME_OBJ_TILE) {
                return false;
            }
        }
    }

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
	if (!tile) {
		return false;
	}

	if (tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
		return false;
	}

	// DEBUG: Log all polygon contacts
	static float polyLogTimer = 0.0f;
	polyLogTimer += 0.016f;
	bool shouldLogPoly = (polyLogTimer >= 0.5f);
	
	if (contact.normal.has_value() && contact.normal->y > kGroundNormalThreshold) {
		if (shouldLogPoly) {
			polyLogTimer = 0.0f;
			float ny = contact.normal.value().y;
			printf("[GROUND] Tile %d: normal.y=%.4f YES\n", tile->getTileIndex(), ny);
		}
		return true;
	}

	// Resolver-provided separation is a useful fallback when SAT normals on slopes are noisy.
	if (contact.separation.has_value() && contact.separation->y < -kGroundNormalThreshold) {
		if (shouldLogPoly) {
			polyLogTimer = 0.0f;
			float sep = contact.separation->y;
			printf("[GROUND] Tile %d: sep=%.4f YES\n", tile->getTileIndex(), sep);
		}
		return true;
	}

    // Sampling fallback may be necessary on shallow polygon slopes even when the contact
    // is not flagged as overlapping.  The original implementation would skip sampling
    // if the contact was not overlapping and the normal was missing or pointed away
    // from the character.  However, some convex polygon tiles (especially those with
    // vertices ordered clockwise) can produce contacts with no normal or with a
    // downward‑facing normal even when they are valid walkable surfaces.  In those
    // situations the character would repeatedly lose and regain ground contact when
    // moving up or down the slope, causing visible “thrashing” in the physics
    // simulation.  To make ground detection more robust for polygonal surfaces, we
    // always attempt a support sample when we have a potential contact.  The
    // subsequent support sampling and supportDelta check will ensure that only
    // surfaces near the character’s feet are treated as ground.  Walls and ceilings
    // still return false if the sampled support is too far away or does not exist.

	const Collidable* selfCollidable = contact.selfCollidable;
	const Collidable* tileCollidable = contact.otherCollidable ? contact.otherCollidable : tile->getCollidable();
	if (!selfCollidable) {
		if (shouldLogPoly) {
			polyLogTimer = 0.0f;
			printf("[GROUND] Tile %d: no selfCollidable\n", tile->getTileIndex());
		}
		return false;
	}

	vector2 selfMin(0.0f, 0.0f);
	vector2 selfMax(0.0f, 0.0f);
	if (!tileCollidable || !tryGetCollidableBounds(selfCollidable, selfMin, selfMax)) {
		if (shouldLogPoly) {
			polyLogTimer = 0.0f;
			printf("[GROUND] Tile %d: no bounds\n", tile->getTileIndex());
		}
		return false;
	}

    // Sample the supporting surface at several points across the width of the character.
    // Sampling only at the horizontal midpoint can miss narrow polygon slopes when the
    // midpoint happens to lie outside the polygon’s horizontal span.  To improve
    // robustness we take multiple samples along the X‑axis and treat the contact as
    // ground if any sample finds a valid support within the allowed vertical
    // tolerance.
	const float bodyBottom = selfMax.y;
	const float bodyWidth = selfMax.x - selfMin.x;
	// Denser sampling across the character width and small horizontal sweep
	// to avoid missing narrow polygon spans or slight misalignments.
	const float sampleFractions[] = { 0.10f, 0.25f, 0.50f, 0.75f, 0.90f };
	const float sampleOffsets[] = { -5.0f, -3.0f, -1.0f, 0.0f, 1.0f, 3.0f, 5.0f };
    
		if (shouldLogPoly) {
			polyLogTimer = 0.0f;
			printf("[GROUND] Tile %d SAMPLING: bottom=%.1f width=%.1f\n",
				tile->getTileIndex(), bodyBottom, bodyWidth);
		}
    
	for (float frac : sampleFractions) {
		for (float off : sampleOffsets) {
			const float sampleX = selfMin.x + (bodyWidth * frac) + off;
			float supportY = 0.0f;
			if (_sampleSupportY(tileCollidable, sampleX, supportY)) {
				const float supportDelta = bodyBottom - supportY;
				if (shouldLogPoly) {
					printf("  frac=%.2f off=%.1f sampleX=%.1f supportY=%.1f delta=%.2f [%.1f,%.1f] %s\n",
						frac, off, sampleX, supportY, supportDelta,
						-kOneWayTopApproachEpsilon, kGroundSupportSnapDistance,
						(supportDelta >= -kOneWayTopApproachEpsilon && supportDelta <= kGroundSupportSnapDistance) ? "YES" : "NO");
				}
				if (supportDelta >= -kOneWayTopApproachEpsilon && supportDelta <= kGroundSupportSnapDistance) {
					return true;
				}
			} else {
				printf("  frac=%.2f off=%.1f sampleX=%.1f NO_SUPPORT\n", frac, off, sampleX);
			}
		}
	}

    // If no sample indicated a valid support within tolerance, this is not ground.
		if (shouldLogPoly) {
			printf("[GROUND] Tile %d: REJECTED\n", tile->getTileIndex());
		}
    return false;
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
	bool bestTileHasSupportSample = false;
	float bestSupportDeltaAbs = std::numeric_limits<float>::max();
	float bestDistance = std::numeric_limits<float>::max();
	std::vector<Tile*> staleTiles;

	float sampleX = 0.0f;
	float bodyBottom = 0.0f;
	bool hasBodySupportSample = false;
	if (Collidable* selfCollidable = this->getCollidable()) {
		vector2 selfMin(0.0f, 0.0f);
		vector2 selfMax(0.0f, 0.0f);
		if (tryGetCollidableBounds(selfCollidable, selfMin, selfMax)) {
			sampleX = selfMin.x + ((selfMax.x - selfMin.x) * 0.5f);
			bodyBottom = selfMax.y;
			hasBodySupportSample = true;
		}
	}

	for (Tile* tile : _groundContacts) {
		if (!tile) {
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

		bool hasSupportSample = false;
		float supportDeltaAbs = std::numeric_limits<float>::max();
		if (hasBodySupportSample) {
			// Use multi-sample sweep across the character width, not just center
			const Collidable* selfCollidable = this->getCollidable();
			if (selfCollidable) {
				vector2 selfMin(0.0f, 0.0f), selfMax(0.0f, 0.0f);
				if (tryGetCollidableBounds(selfCollidable, selfMin, selfMax)) {
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
				(std::fabs(supportDeltaAbs - bestSupportDeltaAbs) <= kFootlineEpsilon && distance < bestDistance)) {
				bestTileHasSupportSample = true;
				bestSupportDeltaAbs = supportDeltaAbs;
				bestDistance = distance;
				bestTile = tile;
			}
			continue;
		}

		if (!bestTileHasSupportSample && distance < bestDistance) {
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
		if (!polygon) {
			return false;
		}
		
		// Even if the polygon isn't marked as "valid" (convex + consistent winding),
		// we should still check if there's support there - the decomposition process
		// may have issues that don't prevent walkable surfaces from being present
		if (!polygon->isValid()) {
			static float logTimer = 0.0f;
			logTimer += 0.016f;
			if (logTimer >= 0.5f) {
				logTimer = 0.0f;
				printf("[POLYGON_INVALID] Attempting support sample on invalid polygon: sampleX=%.2f vertices=%zu convex=%d\n",
					sampleX, polygon->getLocalVertices().size(), (int)polygon->isConvex());
			}
			
			// Try anyway - the surface might still have topography we can sample
			float supportY = 0.0f;
			if (polygon->findTopSurfaceYAtX(sampleX, supportY)) {
				outY = supportY;
				return true;
			}
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
	jump->setExecuteTime(0.5);
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
	falling->setForce(0.0f);

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
    // Provide a collidable for attack animations. Without a collidable the
    // character temporarily loses its collision geometry during an attack,
    // which breaks ground detection and causes the character to enter the
    // falling state after the attack finishes. We reuse a slender hit box
    // similar to idle/running states so the character continues to collide with
    // terrain while attacking. Invincibility from enemies is handled in
    // shouldCollideWith.
    static Square attackHitBox({ -10.0f, kLocomotionFootLocalY - 48.0f }, 20.0f, 48.0f);
    for (unsigned int i = 0; i < attack01Animation->getFrameCount(); i++) {
        Frame* frame = (*attack01Animation)[i];
        if (frame) {
            frame->setCollidable(&attackHitBox);
        }
    }

	Animation* attack02Animation = getAnimation("Attack02", attack02Loaded);
	if (!attack02Loaded) {
		Animations::createFramesForAnimation(attack02Animation, attackSheet, attackDimensions, _spriteManager, 5, 3);
		attack02Animation->setFrameRate(30);
	}
	attack02Animation->setName(attack02->getName());
	attack02Animation->center();
	attack02->setRenderable(attack02Animation);
    // Apply the same hit box to the secondary attack animation frames.
    for (unsigned int i = 0; i < attack02Animation->getFrameCount(); i++) {
        Frame* frame = (*attack02Animation)[i];
        if (frame) {
            frame->setCollidable(&attackHitBox);
        }
    }

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
	else {
		_groundContacts.erase(tile);
		_refreshGroundTile();
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

	if (contact.normal && contact.normal->y < -0.5f) {
		return "JUMP_RELEASED";
	}

	const bool isGroundContact = _isGroundContact(contact);

	if (isGroundContact) {
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

	// Auto-test: simulate holding RIGHT to reproduce slope traversal when
	// environment variable AUTO_SLOPE_TEST=1 is set. Also emit compact telemetry.
	const char* autoTest = std::getenv("AUTO_SLOPE_TEST");
	if (autoTest && std::strcmp(autoTest, "1") == 0) {
		Player* p = Engine2D::getGame()->getPlayerWith(this);
		if (p && p->getController()) {
			Action* right = p->getController()->getAction("RIGHT");
			Action* left = p->getController()->getAction("LEFT");
			if (right) right->setActive(true);
			if (left) left->setActive(false);
		}

		const char* stateName = this->getState() ? this->getState()->getName() : "(null)";
		vector2 pos = this->getPosition();
		vector2 vel = this->getVelocity();
		int groundTileIndex = _tile ? _tile->getTileIndex() : -1;
		printf("[AUTO] state=%s pos={%.2f,%.2f} vel={%.2f,%.2f} ground=%d contacts=%zu\n",
			stateName, pos.x, pos.y, vel.x, vel.y, groundTileIndex, _groundContacts.size());

		// Per-frame sampling telemetry for the current ground tile (if any)
		if (_tile) {
			Collidable* groundCollidable = _tile->getCollidable();
			Collidable* selfCollidable = this->getCollidable();
			if (groundCollidable && selfCollidable) {
				vector2 selfMin(0.0f, 0.0f), selfMax(0.0f, 0.0f);
				if (tryGetCollidableBounds(selfCollidable, selfMin, selfMax)) {
					const float bodyWidth = selfMax.x - selfMin.x;
					const float sampleFracs[] = { 0.10f, 0.25f, 0.50f, 0.75f, 0.90f };
					const float sampleOffsets[] = { -2.0f, -1.0f, 0.0f, 1.0f, 2.0f };
					for (float frac : sampleFracs) {
						for (float off : sampleOffsets) {
							const float sampleX = selfMin.x + (bodyWidth * frac) + off;
							float supportY = 0.0f;
							bool ok = _sampleSupportY(groundCollidable, sampleX, supportY);
							int colType = groundCollidable->getType();
							if (colType == COL_OBJ_POLYGON) {
								const PolygonCollider* poly = (const PolygonCollider*)groundCollidable;
								printf("[AUTO_SAMP] tile=%d frac=%.2f off=%.1f sampleX=%.2f ok=%d supportY=%.2f type=POLY valid=%d convex=%d verts=%zu\n",
									_tile->getTileIndex(), frac, off, sampleX, ok ? 1 : 0, supportY,
									poly->isValid() ? 1 : 0, poly->isConvex() ? 1 : 0, poly->getLocalVertices().size());
							} else {
								printf("[AUTO_SAMP] tile=%d frac=%.2f off=%.1f sampleX=%.2f ok=%d supportY=%.2f type=%d\n",
									_tile->getTileIndex(), frac, off, sampleX, ok ? 1 : 0, supportY, colType);
							}
						}
					}
				}
			}
		}
	}

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

	const bool hasGroundSupport = (_tile != NULL);

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

		if (hasGroundSupport && _tile) {
			Collidable* selfCollidable = this->getCollidable();
			Collidable* groundCollidable = _tile->getCollidable();
			vector2 selfMin(0.0f, 0.0f);
			vector2 selfMax(0.0f, 0.0f);
			if (selfCollidable &&
				groundCollidable &&
				tryGetCollidableBounds(selfCollidable, selfMin, selfMax)) {
				// Sample at multiple points to find the best support, especially important for steep slopes
				const float bodyBottom = selfMax.y;
				const float bodyWidth = selfMax.x - selfMin.x;
				const float sampleFracs[] = { 0.10f, 0.25f, 0.50f, 0.75f, 0.90f };
				const float sampleOffsetsLocal[] = { -5.0f, -3.0f, -1.0f, 0.0f, 1.0f, 3.0f, 5.0f };

				float bestSupportY = std::numeric_limits<float>::lowest();
				bool foundSupport = false;
				float correctionY = 0.0f;
				bool appliedSnap = false;

				for (float frac : sampleFracs) {
					for (float off : sampleOffsetsLocal) {
						const float sampleX = selfMin.x + (bodyWidth * frac) + off;
						float supportY = 0.0f;
						if (_sampleSupportY(groundCollidable, sampleX, supportY)) {
							const float supportDelta = bodyBottom - supportY;

							// Track the best (topmost) supportY (largest Y, closest to body bottom)
							if (!foundSupport || supportY > bestSupportY) {
								bestSupportY = supportY;
								foundSupport = true;
							}

							// Immediate snap if within strict snap distance
							if (supportDelta >= -kOneWayTopApproachEpsilon && supportDelta <= kGroundSupportSnapDistance) {
								correctionY = supportY - bodyBottom;
								appliedSnap = true;
								break;
							}
						} else {
							printf("  frac=%.2f off=%.1f sampleX=%.1f NO_SUPPORT\n", frac, off, sampleX);
						}
					}
					if (appliedSnap) break;
				}

				if (!appliedSnap && foundSupport) {
					// Allow a slightly larger tolerance for non-exact samples
					const float bestSupportDelta = bodyBottom - bestSupportY;
					if (bestSupportDelta <= (kGroundSupportSnapDistance * 2.0f)) {
						correctionY = bestSupportY - bodyBottom;
						appliedSnap = true;
					}
				}

				if (appliedSnap) {
					if (std::fabs(correctionY) > kFootlineEpsilon) {
						this->setPosition(this->getPosition().x, this->getPosition().y + correctionY);
					}

					// Only zero Y velocity if it's pushing into the ground (positive/downward)
					vector2 snappedVelocity = this->getVelocity();
					if (snappedVelocity.y > kHorizontalVelocityEpsilon) {
						snappedVelocity.y = 0.0f;
						this->setVelocity(snappedVelocity);
					}
				}
			}
		}

		if (!groundedForMovement && isAerialState) {
			velocity = this->getVelocity();
			velocity.y = std::min(kNormalFallMaxSpeed, velocity.y + (kGravityAcceleration * time));
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
	if (DEBUGGING && Debug::dbgCollision) {
		static float collisionTelemetryTimer = 0.0f;
		collisionTelemetryTimer += std::max(0.0f, time);
		if (collisionTelemetryTimer >= 0.25f) {
			collisionTelemetryTimer = 0.0f;
			const vector2 pos = this->getPosition();
			const vector2 vel = this->getVelocity();
			const Tile* groundTile = _tile;
			const int groundTileIndex = groundTile ? groundTile->getTileIndex() : -1;
			const std::string groundLayerName =
				(groundTile && !groundTile->getLayerName().empty()) ? groundTile->getLayerName() : "(none)";
			char buffer[320];
			sprintf_s(
				buffer,
				sizeof(buffer),
				"Hero dbg: pos={%.2f,%.2f} vel={%.2f,%.2f} state=%s support=%s contacts=%zu groundTile=%d@%s noGroundT=%.3f dropT=%.3f\n",
				pos.x,
				pos.y,
				vel.x,
				vel.y,
				this->getState() ? this->getState()->getName() : "(null)",
				(_tile != NULL) ? "yes" : "no",
				_groundContacts.size(),
				groundTileIndex,
				groundLayerName.c_str(),
				_timeWithoutGroundContact,
				_dropThroughTimer);
			DEBUG_MSG(buffer);
		}
	}
#endif
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
