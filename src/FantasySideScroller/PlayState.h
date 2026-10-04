// PlayState.h
#pragma once

#include "../GameState.h"
#include "../Cursor.h"

#include "LevelManager.h"
#include "LevelProps.h"
#include "TraversalMechanics.h"

#include <vector>

class Character;
class Boar;
class Font;
class PauseState;

class PlayState : public GameState
{
	void _initHUD();
	void _updateHUD(float dt);
	void _shutdownHUD();

	Player* _player = NULL;
	LevelManager _levelManager;
	TraversalMechanics _traversalMechanics;
	bool _traversalOperatorRegistered = false;
	LevelProps _levelProps;
	bool _interactWasActive = false;
	bool _reloadWasDown = false;

	Character* _playableCharacter = NULL;
	std::vector<Boar*> _boars;
	bool _paused = false;
	PauseState* _pauseState = NULL;
	IRenderer::RenderList* _hudRenderList = NULL;
	Sprite* _healthBarBackground = NULL;
	Sprite* _healthBarFill = NULL;
	Sprite* _staminaBarBackground = NULL;
	Sprite* _staminaBarFill = NULL;
	Sprite* _keyIcon = NULL;
	// Font* _helloWorldText = NULL;
	Cursor* _cursor = NULL;

public:
	PlayState(void);
	virtual ~PlayState(void);

	void onEnter(State* prev);
	bool onExecute(float time);
	void onExit(State* next);
};
