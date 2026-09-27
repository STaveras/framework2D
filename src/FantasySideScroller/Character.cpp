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
	float dropThroughResumeTopY = 0.0f;
	bool hasDropThroughResumeTopY = false;

	auto collectTileBottomY = [&](Tile* tile) {
		if (!tile || !_isOneWayTile(tile)) {
			return;
		}

		Collidable* tileCollidable = tile->getCollidable();
		vector2 tileMin(0.0f, 0.0f);
		vector2 tileMax(0.0f, 0.0f);
		if (!tileCollidable || !Kinematics2D::tryGetActiveBounds(tileCollidable, tileMin, tileMax)) {
			return;
		}

		if (!hasDropThroughResumeTopY || tileMax.y > dropThroughResumeTopY) {
			dropThroughResumeTopY = tileMax.y;
			hasDropThroughResumeTopY = true;
		}
	};

	collectTileBottomY(_tile);

	for (auto itr = _kinematic2DState().groundContacts.begin(); itr != _kinematic2DState().groundContacts.end();) {
		Tile* tile = dynamic_cast<Tile*>(*itr);
		if (_isOneWayTile(tile)) {
			collectTileBottomY(tile);
			itr = _kinematic2DState().groundContacts.erase(itr);
			continue;
		}
		++itr;
	}

	_refreshGroundTile();
	_kinematic2DState().timeWithoutGroundContact = kGroundLossGraceSeconds;

	if (GameObjectState* state = this->getState()) {
		const char* stateName = state->getName();
		if (_isGroundedLocomotionState(stateName) || !strcmp(stateName, "Attack01") || !strcmp(stateName, "Attack02")) {
			this->sendInput("IN_AIR");
		}
		else if (!strcmp(stateName, "Rising") || !strcmp(stateName, "Jump")) {
			this->sendInput("JUMP_RELEASED");
		}
	}

	vector2 velocity = this->getVelocity();
	if (velocity.y < kDropThroughStartDownwardSpeed) {
		velocity.y = kDropThroughStartDownwardSpeed;
		this->setVelocity(velocity);
	}

	if (hasDropThroughResumeTopY) {
		// Keep one-way tiles disabled until our top is fully below the previous
		// one-way tile body. This avoids side pushback when dropping through.
		_kinematic2DState().dropThroughResumeTopY = dropThroughResumeTopY + kOneWayTopApproachEpsilon;
		_kinematic2DState().dropThroughResumePending = true;
	}
	else {
		Collidable* selfCollidable = this->getCollidable();
		vector2 selfMin(0.0f, 0.0f);
		vector2 selfMax(0.0f, 0.0f);
		if (selfCollidable && Kinematics2D::tryGetActiveBounds(selfCollidable, selfMin, selfMax)) {
			_kinematic2DState().dropThroughResumeTopY = selfMax.y + kOneWayTopApproachEpsilon;
			_kinematic2DState().dropThroughResumePending = true;
		}
	}

	this->requestDropThrough(
		kDropThroughDurationSeconds,
		_kinematic2DState().dropThroughResumeTopY,
		_kinematic2DState().dropThroughResumePending);

	this->setPosition(this->getPosition().x, this->getPosition().y + kDropThroughStartNudge);
}

bool Character::_canCollideWithOneWayTile(const Tile* tile) const
{
	if (!_isOneWayTile(tile)) {
		return true;
	}

	if (_kinematic2DState().dropThroughTimer > 0.0f) {
		return false;
	}

	if (_kinematic2DState().dropThroughResumePending) {
		return false;
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
	const float sampleX = bodyMin.x + ((bodyMax.x - bodyMin.x) * 0.5f);

	float supportY = 0.0f;
	if (!_sampleSupportY(tileCollidable, sampleX, supportY)) {
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
