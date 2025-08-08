// Character.h

#pragma once

#include "../GameObject.h"
#include "../Tile.h"
#include "../AnimationUtils.h"
#include "../CollisionEvent.h"

#include "Constants.h"
#include "Resources.h"

class Character : public GameObject
{
	Tile* _tile = NULL; // The tile the character is on

	// Initialize animation states and hitboxes
	void _initStates();
	// Load or set up state transitions
	void _initTransitions();
	// Configure collision callback
	void _setupCollisionHandler();

public:
	Character(void);
	virtual ~Character(void);

	void update(float time) override;
};