#pragma once

#include "Event.h"
//#include "GameObject.h"

#include "Types.h"
#include "Cyclable.h"

// I don't think there should be a Player class, anymore :(
// We should move to the "Controller" paradigm

class Controller;
class GameObject;

class Player : Cyclable
{
	friend class GameState;

	vector2 _heading;

	Controller* _pad;
	GameObject* _object;

	void _OnKeyPress(const Event& evt);
	void _OnKeyRelease(const Event& evt);

public:
	vector2 getHeading(void) const { return _heading; }

	GameObject* getGameObject(void) const { return _object; }
	void setGameObject(GameObject* object) { _object = object; }

	Controller* getController(void) const { return _pad; } // Could be an AI (e.g. machine) Controller!
	void setController(Controller* pad) { _pad = pad; }

	void start(void);
	void update(float time);
	void shutdown(void);
};