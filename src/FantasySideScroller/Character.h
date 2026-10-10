// Character.h

#pragma once

#include "../Actor.h"
#include "../Tile.h"
#include "../AnimationUtils.h"
#include "../BlinkFlash.h"

#include "Constants.h"
#include "Resources.h"

#include <vector>

class PlayerController;

class Character : public Actor
{
public:
	// Intent action slots a Controller fills for a Character. Horizontal
	// movement comes from Intent::move.x.
	enum Action
	{
		ACTION_JUMP,
		ACTION_DOWN,
		ACTION_RUN
	};

	// Bind the FantasySideScroller input actions (JUMP, DOWN, RUN, LEFT,
	// RIGHT) to a player controller.
	static void bindPlayerActions(PlayerController& controller);

private:
	Tile* _tile = NULL; // The tile the character is on
	float _dropThroughSupportY = 0.0f;
	float _health = 100.0f;
	float _maxHealth = 100.0f;
	float _stamina = 100.0f;
	float _maxStamina = 100.0f;
	bool _runBoostActive = false;
	float _longJumpMomentumSpeed = 0.0f;
	int _longJumpMomentumDirection = 0;
	bool _longJumpMomentumActive = false;
	BlinkFlash _damageFlash;
	// While positive, input steering is suspended so a knockback keeps its velocity.
	float _knockbackRemaining = 0.0f;
	// Reused by _findGroundSupportTile so support scans do not allocate each tick.
	std::vector<GameObject*> _supportCandidates;

protected:
	// Initialize animation states and hitboxes
	void _initStates();
	// Load or set up state transitions
	void _initTransitions();
	Physical::KinematicState2D& _kinematic2DState();
	const Physical::KinematicState2D& _kinematic2DState() const;
	bool _isOneWayTile(const Tile* tile) const;
	bool _isDropThroughRequested();
	void _startDropThrough();
	bool _canCollideWithOneWayTile(const Tile* tile) const;
	bool _isGroundContact(const CollisionContact& contact) const;
	bool _isWallBlockingContact(const CollisionContact& contact, int horizontalIntent, float footY, float maxStepUpDistance) const;
	bool _isGroundedLocomotionState(const char* stateName) const;
	bool _canTriggerGroundCollisionFromFalling() const;
	int _getHorizontalInput() const;
	int _getHorizontalIntent() const;
	bool _isRunRequested() const;
	bool _getStateFootLocalY(const GameObjectState* state, float& outFootY) const;
	void _refreshGroundTile();
	bool _sampleSupportY(const Collidable* collidable, float sampleX, float& outY) const;
	bool _findSupportOnTile(const Tile* tile, float footY, float maxSnapDistance, float& outSupportY) const;
	Tile* _findGroundSupportTile(float footY, float maxSnapDistance, float& outSupportY, int* outSupportSampleSource = NULL);
	virtual void handleCollisionContact(const CollisionContact& contact) override;
	virtual void onStateDidEnter(State* previous, State* current) override;
	virtual const char* mapCollisionToCommand(const CollisionContact& contact) const override;
	void onUpdate(float time) override;

public:
	Character(void);
	virtual ~Character(void);
	void resetForRespawn(void);

	float getHealth() const { return _health; }
	float getMaxHealth() const { return _maxHealth; }
	float getHealthNormalized() const { return (_maxHealth > 0.0f) ? (_health / _maxHealth) : 0.0f; }
	void addHealth(float amount);

	// Launches the character with the given velocity (pixels/second, +y down) and
	// suspends input steering for lockSeconds, e.g. when thrown off by spikes.
	void applyKnockback(vector2 velocity, float lockSeconds);
	bool isKnockedBack() const { return _knockbackRemaining > 0.0f; }

	float getStamina() const { return _stamina; }
	float getMaxStamina() const { return _maxStamina; }
	float getStaminaNormalized() const { return (_maxStamina > 0.0f) ? (_stamina / _maxStamina) : 0.0f; }
	bool isRunBoostActive() const { return _runBoostActive; }
	void addStamina(float amount);
	float getHorizontalSpeed() const {
		const float vx = this->getVelocity().x;
		return (vx >= 0.0f) ? vx : -vx;
	}

	// Facing is the sign of the current animation's X scale; states carry it into the next.
	bool isFacingLeft() const;
	void setFacingLeft(bool facingLeft);

	bool shouldCollideWith(const GameObject& other) const override;
};
