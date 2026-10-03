// Actor.h
// A GameObject that can be possessed by a Controller. Each tick the Actor asks
// its Controller for an Intent before running its own update, so game code
// reads getIntent() instead of polling input directly, and a player or an AI
// can drive the same Actor.

#pragma once

#include "GameObject.h"
#include "Intent.h"

class Controller;

class Actor : public GameObject
{
	Controller* _controller = nullptr;
	Intent _intent;

protected:
	// Per-tick behavior; runs after the Intent is refreshed. Defaults to the
	// plain GameObject update.
	virtual void onUpdate(float time) { GameObject::update(time); }

public:
	explicit Actor(GAME_OBJ_TYPE type) : GameObject(type) {}

	// Take control of this Actor (nullptr releases it). The Intent resets.
	void possess(Controller* controller);
	Controller* getController(void) const { return _controller; }
	bool isPossessed(void) const { return _controller != nullptr; }

	const Intent& getIntent(void) const { return _intent; }

	void update(float time) override final;
};
