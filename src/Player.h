#pragma once
// todo: below
class GameObject;
class VirtualGamePad;
class Player
{
	friend class GameState;

	VirtualGamePad* _pad;
	GameObject* _object;

public:
	GameObject* GetGameObject(void) const { return _object; }
	void SetGameObject(GameObject* object) { _object = object; }

	VirtualGamePad* GetGamePad(void) const { return _pad; }
	void SetGamePad(VirtualGamePad* pad) { _pad = pad; }

	//void Setup(void);
	//void Update(float time);
	//void Shutdown(void);
};