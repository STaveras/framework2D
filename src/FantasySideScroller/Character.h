// Character.h

#pragma once

#include "../GameObject.h"
#include "../Tile.h"
#include "../AnimationUtils.h"

#include "Constants.h"
#include "Resources.h"

#include <unordered_set>
#include <unordered_map>

class Character : public GameObject
{
	Tile* _tile = NULL; // The tile the character is on
	std::unordered_set<Tile*> _groundContacts;
	std::unordered_map<Tile*, int> _groundContactFrameCount;  // Hysteresis: frames since contact started
	float _timeWithoutGroundContact = 0.0f;
	float _pendingTransitionFootCorrection = 0.0f;
	float _dropThroughTimer = 0.0f;
	float _fallingLandingDebounceTimer = 0.0f;
	float _stamina = 100.0f;
	float _maxStamina = 100.0f;
	bool _runBoostActive = false;
	float _longJumpMomentumSpeed = 0.0f;
	int _longJumpMomentumDirection = 0;
	bool _longJumpMomentumActive = false;
	float _telemetryPendingWallCorrectionX = 0.0f;
	float _telemetryLastWallCorrectionX = 0.0f;
	int _telemetryPendingGroundContacts = 0;
	int _telemetryPendingWallContacts = 0;
	int _telemetryLastGroundContacts = 0;
	int _telemetryLastWallContacts = 0;

protected:
	// Initialize animation states and hitboxes
	void _initStates();
	// Load or set up state transitions
	void _initTransitions();
	bool _isOneWayTile(const Tile* tile) const;
	bool _isDropThroughRequested() const;
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

public:
	Character(void);
	virtual ~Character(void);
	void resetForRespawn(void);

	float getStamina() const { return _stamina; }
	float getMaxStamina() const { return _maxStamina; }
	float getStaminaNormalized() const { return (_maxStamina > 0.0f) ? (_stamina / _maxStamina) : 0.0f; }
	bool isRunBoostActive() const { return _runBoostActive; }
	float getHorizontalSpeed() const {
		const float vx = this->getVelocity().x;
		return (vx >= 0.0f) ? vx : -vx;
	}

	bool shouldCollideWith(const GameObject& other) const override;
	void update(float time) override;
};
