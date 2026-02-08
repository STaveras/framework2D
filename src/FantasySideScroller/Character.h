// Character.h

#pragma once

#include "../GameObject.h"
#include "../Tile.h"
#include "../AnimationUtils.h"

#include "Constants.h"
#include "Resources.h"

#include <unordered_set>

class Character : public GameObject
{
	Tile* _tile = NULL; // The tile the character is on
	std::unordered_set<Tile*> _groundContacts;
	float _timeWithoutGroundContact = 0.0f;
	float _pendingTransitionFootCorrection = 0.0f;
	float _dropThroughTimer = 0.0f;

	// Initialize animation states and hitboxes
	void _initStates();
	// Load or set up state transitions
	void _initTransitions();
	bool _isOneWayTile(const Tile* tile) const;
	bool _isDropThroughRequested() const;
	void _startDropThrough();
	bool _canCollideWithOneWayTile(const Tile* tile) const;
	bool _isGroundContact(const CollisionContact& contact) const;
	bool _isGroundedLocomotionState(const char* stateName) const;
	int _getHorizontalIntent() const;
	bool _getStateFootLocalY(const GameObjectState* state, float& outFootY) const;
	void _refreshGroundTile();
	bool _sampleSupportY(const Collidable* collidable, float sampleX, float& outY) const;
	bool _findSupportOnTile(const Tile* tile, float footY, float maxSnapDistance, float& outSupportY) const;
	Tile* _findGroundSupportTile(float footY, float maxSnapDistance, float& outSupportY);
	virtual void handleCollisionContact(const CollisionContact& contact) override;
	virtual void onStateDidEnter(State* previous, State* current) override;
	virtual const char* mapCollisionToCommand(const CollisionContact& contact) const override;

public:
	Character(void);
	virtual ~Character(void);

	bool shouldCollideWith(const GameObject& other) const override;
	void update(float time) override;
};
