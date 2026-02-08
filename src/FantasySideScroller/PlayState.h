// PlayState.h
#pragma once

#include "../GameState.h"

#include "LevelManager.h"

class Character;

class PlayState : public GameState
{
	Player* _player = NULL;
	LevelManager _levelManager;

	Character* _playableCharacter = NULL;

public:
	PlayState(void);
	virtual ~PlayState(void);

	void onEnter(State* prev);
	bool onExecute(float time);
	void onExit(State* next);
};
