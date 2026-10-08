// PlayState.h
#pragma once

#include "../GameState.h"
#include "../Cursor.h"
#include "../PlayerController.h"

#include "LevelManager.h"
#include "LevelProps.h"
#include "TraversalMechanics.h"

#include <vector>

class Character;
class Boar;
class Font;
class PauseState;
class GameOverState;

class PlayState : public GameState
{
	void _initHUD();
	void _updateHUD(float dt);
	void _shutdownHUD();
	void _respawnPlayer(void);

	Player* _player = NULL;
	PlayerController _playerController;
	LevelManager _levelManager;
	TraversalMechanics _traversalMechanics;
	bool _traversalOperatorRegistered = false;
	LevelProps _levelProps;
	std::string _mapFileName = "mosswood_hollow.tmj";
	bool _interactWasActive = false;
	bool _reloadWasDown = false;

	Character* _playableCharacter = NULL;
	std::vector<Boar*> _boars;
	bool _paused = false;
	PauseState* _pauseState = NULL;
	GameOverState* _gameOverState = NULL;
	IRenderer::RenderList* _hudRenderList = NULL;
	Image* _healthBarBackground = NULL;
	Image* _healthBarFill = NULL;
	Image* _staminaBarBackground = NULL;
	Image* _staminaBarFill = NULL;
	Image* _keyIcon = NULL;
	// Font* _helloWorldText = NULL;
	Cursor* _cursor = NULL;

public:
	PlayState(void);
	virtual ~PlayState(void);

	void onEnter(State* prev);
	bool onExecute(float time);
	void onExit(State* next);
};
