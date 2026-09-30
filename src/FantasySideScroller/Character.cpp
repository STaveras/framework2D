// Character.cpp

#include "Character.h"
#include "CharacterTuning.h"

#include "../Animation.h"
#include "../GameState.h"
#include "../Kinematics2D.h"
#include "../Square.h"
#include "../Telemetry2D.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace CharacterPrivate
{
Telemetry2D::Runtime& AutoRuntime();
}

namespace {
constexpr float kDamageFlashDuration = 0.64f;
constexpr float kDamageFlashHalfPeriod = 0.08f;
constexpr float kDamageFlashOpacity = 0.85f;
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
	Physical::KinematicConfig2D kinematicConfig = this->getKinematicConfig2D();
	kinematicConfig.enabled = true;
	kinematicConfig.groundNormalThreshold = kGroundNormalThreshold;
	kinematicConfig.wallNormalThreshold = kWallNormalThreshold;
	kinematicConfig.oneWayEpsilon = kOneWayTopApproachEpsilon;
	kinematicConfig.supportProbeFootAboveTolerance = kOneWayTopApproachEpsilon;
	kinematicConfig.supportProbeFootBelowTolerance = kGroundSupportSnapDistance;
	kinematicConfig.minimumSupportSamples = 1;
	kinematicConfig.maxStepHeight = kMaxAutoStepUpDistance;
	kinematicConfig.dropThroughDefaultDuration = kDropThroughDurationSeconds;
	this->setKinematicConfig2D(kinematicConfig);
	this->resetKinematicState2D();
	_maxHealth = kHealthMax;
	_health = _maxHealth;
	_maxStamina = kStaminaMax;
	_stamina = _maxStamina;
	_runBoostActive = false;
}

Character::~Character()
{
	Telemetry2D::shutdown(CharacterPrivate::AutoRuntime());
}

Physical::KinematicState2D& Character::_kinematic2DState()
{
	return this->getKinematicState2D();
}

const Physical::KinematicState2D& Character::_kinematic2DState() const
{
	return this->getKinematicState2D();
}

void Character::resetForRespawn(void)
{
	_tile = NULL;
	_dropThroughSupportY = 0.0f;
	_kinematic2DState().groundContacts.clear();
	this->resetKinematicState2D();
	_runBoostActive = false;
	_longJumpMomentumActive = false;
	_longJumpMomentumDirection = 0;
	_longJumpMomentumSpeed = 0.0f;
	_damageFlashRemaining = 0.0f;
	_damageFlashPhase = 0.0f;
	_damageFlashOn = false;
	_setDamageFlash(false);
	_knockbackRemaining = 0.0f;
	_health = _maxHealth;
	this->setVelocity(vector2(0.0f, 0.0f));
}

bool Character::_isOneWayTile(const Tile* tile) const
{
	return tile &&
		tile->isOneWay();
}

bool Character::_isDropThroughRequested()
{
	Game* game = Engine2D::getGame();
	if (!game) {
		_kinematic2DState().dropThroughJumpWasDown = false;
		return false;
	}

	Player* player = game->getPlayerWith((GameObject*)this);
	if (!player || !player->getController()) {
		_kinematic2DState().dropThroughJumpWasDown = false;
		return false;
	}

	Controller* controller = player->getController();
	Action* jumpAction = controller->getAction("JUMP");
	Action* downAction = controller->getAction("DOWN");
	if (!jumpAction || !downAction) {
		_kinematic2DState().dropThroughJumpWasDown = false;
		return false;
	}

	const bool jumpDown = jumpAction->isActive();
	const bool jumpPressed = jumpDown && !_kinematic2DState().dropThroughJumpWasDown;
	_kinematic2DState().dropThroughJumpWasDown = jumpDown;
	return jumpPressed && downAction->isActive();
}

void Character::_startDropThrough()
{
	_kinematic2DState().dropThroughTimer = kDropThroughDurationSeconds;
	_kinematic2DState().dropThroughResumePending = false;
	// Suppress the departure surface for a bounded time. Lower platforms must
	// remain available even while the body still overlaps the departure tile.
	vector2 bodyMin(0.0f, 0.0f);
	vector2 bodyMax(0.0f, 0.0f);
	_dropThroughSupportY = this->getPosition().y;
	if (Kinematics2D::tryGetActiveBounds(this->getCollidable(), bodyMin, bodyMax)) {
		_dropThroughSupportY = bodyMax.y;
	}

	for (auto itr = _kinematic2DState().groundContacts.begin(); itr != _kinematic2DState().groundContacts.end();) {
		Tile* tile = dynamic_cast<Tile*>(*itr);
		if (_isOneWayTile(tile)) {
			_kinematic2DState().groundContactFrameCount.erase(tile);
			itr = _kinematic2DState().groundContacts.erase(itr);
			continue;
		}
		++itr;
	}

	_refreshGroundTile();
	_kinematic2DState().timeWithoutGroundContact = kGroundLossGraceSeconds;

	if (GameObjectState* state = this->getState()) {
		const char* stateName = state->getName();
		if (_isGroundedLocomotionState(stateName) ||
			!strcmp(stateName, "Attack01") || !strcmp(stateName, "Attack02") ||
			!strcmp(stateName, "Rising") || !strcmp(stateName, "Jump")) {
			// A confirmed drop must enter the airborne state even when the
			// current animation has no IN_AIR transition (Landing and attacks).
			this->setState("Falling");
		}
	}

	vector2 velocity = this->getVelocity();
	if (velocity.y < kDropThroughStartDownwardSpeed) {
		velocity.y = kDropThroughStartDownwardSpeed;
		this->setVelocity(velocity);
	}

	this->setPosition(this->getPosition().x, this->getPosition().y + kDropThroughStartNudge);
}

bool Character::_canCollideWithOneWayTile(const Tile* tile) const
{
	if (!_isOneWayTile(tile)) {
		return true;
	}

	if (this->getVelocity().y < 0.0f) {
		return false;
	}

	Collidable* selfCollidable = ((Character*)this)->getCollidable();
	Collidable* tileCollidable = ((Tile*)tile)->getCollidable();
	if (!selfCollidable || !tileCollidable || !selfCollidable->isActive() || !tileCollidable->isActive()) {
		return false;
	}

	if (selfCollidable->getType() != COL_OBJ_SQUARE) {
		return false;
	}

	Square* bodySquare = (Square*)selfCollidable;
	const vector2 bodyMin = bodySquare->getMin();
	const vector2 bodyMax = bodySquare->getMax();
	float supportY = 0.0f;
	if (!Kinematics2D::sampleSupportYInOverlap(tileCollidable, bodyMin.x, bodyMax.x, supportY)) {
		return false;
	}

	if (_kinematic2DState().dropThroughTimer > 0.0f &&
		supportY <= _dropThroughSupportY + kOneWayTopApproachEpsilon) {
		return false;
	}

	const float bodyTop = bodyMin.y;
	return bodyTop < (supportY + kOneWayTopApproachEpsilon);
}

bool Character::shouldCollideWith(const GameObject& other) const
{
    // During attacks, suppress gameplay contact handling for non-terrain
    // objects. The physics solver still keeps dynamic bodies separated.
    if (const GameObjectState* state = this->getState()) {
        const char* stateName = state->getName();
        if (stateName && (!std::strcmp(stateName, "Attack01") || !std::strcmp(stateName, "Attack02"))) {
            // Only dispatch gameplay contacts with tiles while attacking.
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

void Character::addStamina(float amount)
{
	_stamina = std::max(0.0f, std::min(_maxStamina, _stamina + amount));
}

void Character::applyKnockback(vector2 velocity, float lockSeconds)
{
	if (_health <= 0.0f) {
		return;
	}

	GameObjectState* state = this->getState();
	const char* stateName = state ? state->getName() : "";
	const bool aerial =
		!strcmp(stateName, "Rising") || !strcmp(stateName, "Jump") || !strcmp(stateName, "Falling");
	// Leave the ground (and any attack pose) so gravity and the push both apply.
	if (!aerial && strcmp(stateName, "Dead") != 0) {
		if (GameObjectState* falling = this->getState("Falling")) {
			this->setState(falling);
		}
	}

	_longJumpMomentumActive = false;
	_longJumpMomentumDirection = 0;
	_longJumpMomentumSpeed = 0.0f;
	_runBoostActive = false;
	_knockbackRemaining = std::max(_knockbackRemaining, lockSeconds);
	this->setVelocity(velocity);
}

void Character::addHealth(float amount)
{
	const float previousHealth = _health;
	_health = std::max(0.0f, std::min(_maxHealth, _health + amount));
	if (_health < previousHealth) _startDamageFlash();
}

void Character::_setDamageFlash(bool enabled)
{
	for (auto stateIt = begin(); stateIt != end(); ++stateIt) {
		GameObjectState* state = static_cast<GameObjectState*>(*stateIt);
		if (Renderable* renderable = state->getRenderable()) {
			renderable->setFlash(enabled, 0xFFFF5555, kDamageFlashOpacity);
		}
	}
}

void Character::_startDamageFlash()
{
	_damageFlashRemaining = kDamageFlashDuration;
	_damageFlashPhase = kDamageFlashHalfPeriod;
	_damageFlashOn = true;
	_setDamageFlash(true);
}

void Character::_updateDamageFlash(float time)
{
	if (_damageFlashRemaining <= 0.0f) return;

	_damageFlashRemaining = std::max(0.0f, _damageFlashRemaining - time);
	_damageFlashPhase -= time;
	while (_damageFlashPhase <= 0.0f && _damageFlashRemaining > 0.0f) {
		_damageFlashOn = !_damageFlashOn;
		_damageFlashPhase += kDamageFlashHalfPeriod;
	}
	if (_damageFlashRemaining <= 0.0f) {
		_damageFlashOn = false;
		_setDamageFlash(false);
	} else {
		_setDamageFlash(_damageFlashOn);
	}
}
