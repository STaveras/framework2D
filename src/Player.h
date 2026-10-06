#pragma once

#include "Event.h"
//#include "GameObject.h"

#include "Types.h"
#include "Cyclable.h"
#include "InputMap.h"

// I don't think there should be a Player class, anymore :(
// We should move to the "Controller" paradigm

class GameObject;

class Player : public InputMap::EventListener
{
	friend class GameState;

	vector2 _heading;

	InputMap* _pad = NULL;
	GameObject* _object = NULL;

	void onButtonPressed(const Event& evt);
	void onButtonReleased(const Event& evt);
	void onButtonDown(const Event& evt);
	void onButtonUp(const Event& evt);

public:
	vector2 getHeading(void) const { return _heading; }
	void getHeading(vector2 heading) { _heading = heading; }

	GameObject* getGameObject(void) const { return _object; }
	void setGameObject(GameObject* object) { _object = object; }

	InputMap* getInputMap(void) const { return _pad; }
	void setInputMap(InputMap* pad) { _pad = pad; }

	void start(void);
	void update(float time);
	void finish(void);
};
