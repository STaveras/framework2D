// PlayState.h
#pragma once

#include "../GameState.h"
#include "../Sprite.h"

#include "LevelManager.h"
#include "TraversalMechanics.h"

class Character;

class PlayState : public GameState
{
	void _initHUD();
	void _updateHUD(float dt);
	void _shutdownHUD();

	Player* _player = NULL;
	LevelManager _levelManager;
	TraversalMechanics _traversalMechanics;
	bool _traversalOperatorRegistered = false;

	Character* _playableCharacter = NULL;
	IRenderer::RenderList* _hudRenderList = NULL;
	Image* _staminaBarBackground = NULL;
	Image* _staminaBarFill = NULL;
	Image* _timerBarBackground = NULL;
	Image* _timerBarFill = NULL;

public:
	PlayState(void);
	virtual ~PlayState(void);

	void onEnter(State* prev);
	bool onExecute(float time);
	void onExit(State* next);
};
