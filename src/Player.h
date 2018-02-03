#pragma once

#include "Event.h"
#include "GameObject.h"

class VirtualGamePad;
class Player
{
	friend class GameState;

	VirtualGamePad* _pad;
	GameObject* _object;

	void _OnKeyPress(const Event& evt);
  void _OnKeyRelease(const Event & evt);

public:
	GameObject* getGameObject(void) const { return _object; }
  void setGameObject(GameObject* object) { _object = object; }

	VirtualGamePad* getGamePad(void) const { return _pad; }
	void setGamePad(VirtualGamePad* pad) { _pad = pad; }

	void setup(void);
	//void update(float time);
	void shutdown(void);
};