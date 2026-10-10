// PlayState.h
#pragma once

#include "../GameState.h"
#include "../Cursor.h"
#include "../PlayerController.h"

#include "LevelManager.h"
#include "LevelProps.h"
#include "TraversalMechanics.h"

#include <memory>
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

	PlayerController _playerController;
	LevelManager _levelManager;
	TraversalMechanics _traversalMechanics;
	bool _traversalOperatorRegistered = false;
	LevelProps _levelProps;
	std::string _mapFileName = "mosswood_hollow.tmj";
	bool _reloadWasDown = false;
	// Set by a SectionEnd/SectionBegin exit: the next map's destination to spawn beside,
	// and the character's outgoing height and facing.
	std::string _sectionEntryName;
	float _sectionEntryCharacterY = 0.0f;
	bool _sectionEntryFacingLeft = false;

	Character* _playableCharacter = NULL;
	std::vector<Boar*> _boars;
	bool _paused = false;
	// The overlays pushed on top of this state; reused across pauses and deaths.
	std::unique_ptr<PauseState> _pauseState;
	std::unique_ptr<GameOverState> _gameOverState;
	IRenderer::RenderList* _hudRenderList = NULL;
	Image* _healthBarBackground = NULL;
	Image* _healthBarFill = NULL;
	Image* _staminaBarBackground = NULL;
	Image* _staminaBarFill = NULL;
	Image* _keyIcon = NULL;
	// Font* _helloWorldText = NULL;
	std::unique_ptr<Cursor> _cursor;

public:
	PlayState(void);
	virtual ~PlayState(void);

	void onEnter(State* prev);
	bool onExecute(float time);
	void onExit(State* next);
};
