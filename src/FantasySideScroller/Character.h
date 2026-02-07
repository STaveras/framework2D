// Character.h

#pragma once

#include "../GameObject.h"
#include "../Tile.h"
#include "../AnimationUtils.h"

#include "Constants.h"
#include "Resources.h"

class Character : public GameObject
{
	Tile* _tile = NULL; // The tile the character is on

	// Initialize animation states and hitboxes
	void _initStates();
	// Load or set up state transitions
	void _initTransitions();
	virtual void handleCollisionContact(const CollisionContact& contact) override;
	virtual const char* mapCollisionToCommand(const CollisionContact& contact) const override;

public:
	Character(void);
	virtual ~Character(void);

	void update(float time) override;
};
