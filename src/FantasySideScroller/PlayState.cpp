// File: PlayState.cpp
// Author: Stanley Taveras
// Created: 2/18/2010
// Modified: 11/13/2024

#include "PlayState.h"

#include "../Camera.h"
#include "../Debug.h"
#include "../Font.h"
#include "../Gamepad.h"
#include "../IInput.h"
#include "../Sprite.h"
#include "../Animation.h"
#include "../Cursor.h"
#include "FantasySideScroller.h"
#include "../InputMap.h"
#include "PauseState.h"
#include "GameOverState.h"

#include "Constants.h"
#include "Character.h"
#include "Boar.h"
#include "../StrUtils.h"
#include "../System.h"

namespace {
constexpr float kHUDPaddingX = 12.0f;
constexpr float kHUDPaddingY = 12.0f;
constexpr float kHUDBackgroundWidth = 104.0f;
constexpr float kHUDBackgroundHeight = 10.0f;
constexpr float kHUDFillInset = 2.0f;
constexpr float kHUDFillMaxWidth = 100.0f;
constexpr float kHUDFillHeight = 6.0f;
constexpr float kHUDTextOffsetY = 10.0f;
constexpr float kHUDTextScale = 1.0f;
constexpr float kHUDStaminaOffsetY = kHUDBackgroundHeight;
constexpr float kHUDStaminaHeight = 1.0f;
constexpr float kTraversalDefaultTimeLimitSeconds = 75.0f;
}

PlayState::PlayState()
    : _playableCharacter(nullptr)
{
	// AUTO_START_MAP=<file>.tmj starts on another map (benchmarks, replays).
	std::string owned;
	const char* startMap = System::getenv_platform("AUTO_START_MAP", owned);
	if (startMap && startMap[0] != '\0') {
		_mapFileName = startMap;
	}
}

PlayState::~PlayState() {
	if (_playableCharacter || _hudRenderList) {
		onExit(nullptr);
	}
	// The pause and game-over overlays are freed with this state
	// (ProgramStack never deletes states).
}

void PlayState::_initHUD()
{
	IRenderer* renderer = Engine2D::getRenderer();
	if (!renderer) {
		return;
	}

	if (!_hudRenderList) {
    	_hudRenderList = renderer->createRenderList(true);
	}

	if (!_healthBarBackground) {
		_healthBarBackground = new Image(BasePath("pixel.bmp").c_str());
		_healthBarBackground->setTint(0xAA101018);
		_healthBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);
		_healthBarBackground->setOffset(vector2(0.0f, 0.0f));
		_healthBarBackground->setVisibility(true);
		_hudRenderList->push_back(_healthBarBackground);
	}

	if (!_healthBarFill) {
		_healthBarFill = new Image(BasePath("pixel.bmp").c_str());
		_healthBarFill->setTint(0xFF32D060);
		_healthBarFill->setScale(kHUDFillMaxWidth, kHUDFillHeight);
		_healthBarFill->setOffset(vector2(0.0f, 0.0f));
		_healthBarFill->setVisibility(true);
		_hudRenderList->push_back(_healthBarFill);
	}

	if (!_staminaBarBackground) {
		_staminaBarBackground = new Image(BasePath("pixel.bmp").c_str());
		_staminaBarBackground->setTint(0xAA101018);
		_staminaBarBackground->setScale(kHUDBackgroundWidth, kHUDStaminaHeight);
		_staminaBarBackground->setOffset(vector2(0.0f, 0.0f));
		_staminaBarBackground->setVisibility(true);
		_hudRenderList->push_back(_staminaBarBackground);
	}

	if (!_staminaBarFill) {
		_staminaBarFill = new Image(BasePath("pixel.bmp").c_str());
		_staminaBarFill->setTint(0xFF38A8E8);
		_staminaBarFill->setScale(kHUDFillMaxWidth, kHUDStaminaHeight);
		_staminaBarFill->setOffset(vector2(0.0f, 0.0f));
		_staminaBarFill->setVisibility(true);
		_hudRenderList->push_back(_staminaBarFill);
	}

	// if (!_helloWorldText) {
	// 	_helloWorldText = new Font();
	// 	const std::string fontPath = BasePath("Font/monogram/bitmap/monogram-bitmap.json");
	// 	if (_helloWorldText->loadFromJSON(fontPath)) {
	// 		_helloWorldText->setText("HelloWorld\nHello World");
	// 		_helloWorldText->setTint(0xFFFFFFFF);
	// 		_helloWorldText->setScale(kHUDTextScale, kHUDTextScale);
	// 		_helloWorldText->setOffset(vector2(0.0f, 0.0f));
	// 		_helloWorldText->setVisibility(true);
	// 		_hudRenderList->push_back(_helloWorldText);
	// 	}
	// 	else {
	// 		DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
	// 		SAFE_DELETE(_helloWorldText);
	// 	}
	// }

	if (!_keyIcon) {
		// The key tile from Tiles.png, shown while the player holds a key.
		_keyIcon = new Image(BasePath("Assets/Tiles.png").c_str(), 0, RECT{ 240, 320, 256, 336 });
		_keyIcon->setVisibility(false);
		_hudRenderList->push_back(_keyIcon);
	}

	// No cursor without a pointer (touch-only iOS has no mouse).
	IInput* input = Engine2D::getInput();
	if (!_cursor && (!input || input->getMouse())) {
		_cursor = std::make_unique<Cursor>();
		if (_cursor->load(BasePath("cursors.png"))) {
			_cursor->getImage()->setVisibility(true);
			_hudRenderList->push_back(_cursor->getImage());
		}
		else {
			_cursor.reset();
		}
	}
}

void PlayState::_updateHUD(float dt)
{
	(void)dt;

	if (!_healthBarBackground || !_healthBarFill || !_staminaBarBackground || !_staminaBarFill) {
		return;
	}

	const vector2 hudOrigin(kHUDPaddingX, kHUDPaddingY);

	_healthBarBackground->setPosition(hudOrigin);
	_healthBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);

	const float normalizedHealth = _playableCharacter ? _playableCharacter->getHealthNormalized() : 0.0f;
	float clampedHealth = normalizedHealth;
	if (clampedHealth < 0.0f) {
		clampedHealth = 0.0f;
	}
	else if (clampedHealth > 1.0f) {
		clampedHealth = 1.0f;
	}

	const float healthFillWidth = kHUDFillMaxWidth * clampedHealth;
	const vector2 healthFillOrigin = hudOrigin + vector2(kHUDFillInset, kHUDFillInset);
	_healthBarFill->setPosition(healthFillOrigin);
	_healthBarFill->setScale(healthFillWidth, kHUDFillHeight);
	_healthBarFill->setVisibility(healthFillWidth > 0.0f);

	const vector2 staminaOrigin = hudOrigin + vector2(0.0f, kHUDStaminaOffsetY);
	_staminaBarBackground->setPosition(staminaOrigin);
	_staminaBarBackground->setScale(kHUDBackgroundWidth, kHUDStaminaHeight + 1.0f);

	const float normalizedStamina = _playableCharacter ? _playableCharacter->getStaminaNormalized() : 0.0f;
	float clampedStamina = normalizedStamina;
	if (clampedStamina < 0.0f) {
		clampedStamina = 0.0f;
	}
	else if (clampedStamina > 1.0f) {
		clampedStamina = 1.0f;
	}

	const float staminaFillWidth = kHUDFillMaxWidth * clampedStamina;
	const vector2 staminaFillOrigin = staminaOrigin + vector2(kHUDFillInset, 0.0f);
	_staminaBarFill->setPosition(staminaFillOrigin);
	_staminaBarFill->setScale(staminaFillWidth, kHUDStaminaHeight);
    _staminaBarFill->setVisibility(staminaFillWidth > 0.0f);

	if (_keyIcon) {
		_keyIcon->setPosition(hudOrigin + vector2(kHUDBackgroundWidth + 4.0f, -3.0f));
		_keyIcon->setVisibility(_levelProps.getKeyCount() > 0);
	}
    
    // if (_helloWorldText) {
    //     _helloWorldText->setPosition(hudOrigin + vector2(0.0f, kHUDTextOffsetY));
    // }
}

void PlayState::_shutdownHUD()
{
	if (_hudRenderList) {
		if (_healthBarBackground) {
			_hudRenderList->remove(_healthBarBackground);
		}
		if (_healthBarFill) {
			_hudRenderList->remove(_healthBarFill);
		}
		if (_staminaBarBackground) {
			_hudRenderList->remove(_staminaBarBackground);
		}
		if (_staminaBarFill) {
			_hudRenderList->remove(_staminaBarFill);
		}
		if (_keyIcon) {
			_hudRenderList->remove(_keyIcon);
		}
		// if (_helloWorldText) {
		// 	_hudRenderList->remove(_helloWorldText);
		// }
		if (_cursor && _cursor->getImage()) {
			_hudRenderList->remove(_cursor->getImage());
		}

		if (IRenderer* renderer = Engine2D::getRenderer()) {
			renderer->destroyRenderList(_hudRenderList);
		}
		_hudRenderList = NULL;
	}

	SAFE_DELETE(_healthBarBackground);
	SAFE_DELETE(_healthBarFill);
	SAFE_DELETE(_staminaBarBackground);
	SAFE_DELETE(_staminaBarFill);
	SAFE_DELETE(_keyIcon);
	// SAFE_DELETE(_helloWorldText);
	_cursor.reset();
}

// Restore the playable character to a fresh spawn: reset the collision world,
// the boar, and the character (full health, back in a Falling state at the
// spawn point). Shared by the debug R key and the game-over RETRY prompt.
void PlayState::_respawnPlayer(void)
{
	if (!_playableCharacter) {
		return;
	}

	_collisionSystem.reset();
	for (Boar* boar : _boars) {
		if (boar) boar->reset();
	}
	_levelProps.resetPlatforms();

	_playableCharacter->clearEvents();
	_playableCharacter->resetForRespawn();
	_playableCharacter->setState(_playableCharacter->getState("Falling"));

	if (_levelManager.hasSpawnPoint()) {
		_playableCharacter->setPosition(_levelManager.getSpawnPoint());
	}
	else {
		_playableCharacter->setPosition(_playableCharacter->getPosition().x, -120.0f);
	}

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision) {
		const vector2 pos = _playableCharacter->getPosition();
		char buffer[192];
		sprintf_s(buffer, sizeof(buffer),
			"Respawn: pos={%.2f,%.2f} state=%s\n",
			pos.x,
			pos.y,
			_playableCharacter->getState() ? _playableCharacter->getState()->getName() : "(null)");
		DEBUG_MSG(buffer);
	}
#endif
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	// Preferred: map-declared tilesets from the .tmj file.
	_levelManager.initialize(_mapFileName.c_str(), vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);

	_playableCharacter = new Character;
	vector2 spawnPoint = START_POSITION;
	if (_levelManager.hasSpawnPoint()) {
		spawnPoint = _levelManager.getSpawnPoint();
		if (!_sectionEntryName.empty()) {
			spawnPoint = _levelManager.getSpawnPointNearDestination(_sectionEntryName, _sectionEntryCharacterY);
		}
	}
	_playableCharacter->setPosition(spawnPoint);
	if (!_sectionEntryName.empty()) {
		// Enter the next section facing the way the character left the last one.
		_playableCharacter->setFacingLeft(_sectionEntryFacingLeft);
	}
	_sectionEntryName.clear();

#if _DEBUG
	{
		char buffer[192];
		sprintf_s(buffer, sizeof(buffer),
			"PlayState spawn: hasSpawn=%s pos={%.2f,%.2f}\n",
			_levelManager.hasSpawnPoint() ? "true" : "false",
			spawnPoint.x,
			spawnPoint.y);
		DEBUG_MSG(buffer);
	}
#endif

	_objectManager.addObject("Hero", _playableCharacter);
	_levelProps.initialize(_levelManager.getTileMaps());
	const std::vector<LevelEnemyDescriptor>& enemyDescriptors = _levelManager.getEnemyDescriptors();
	_boars.reserve(enemyDescriptors.size());
	for (size_t i = 0; i < enemyDescriptors.size(); ++i) {
		const LevelEnemyDescriptor& descriptor = enemyDescriptors[i];
		if (!StrUtils::IEquals(descriptor.typeName, "boar")) {
			continue;
		}

		Boar* boar = new Boar(_objectManager, *_playableCharacter, descriptor.position);
		_boars.push_back(boar);
		const int objectId = (descriptor.objectId >= 0) ? descriptor.objectId : (int)i + 1;
		const std::string objectName = "Boar_" + std::to_string(objectId);
		_objectManager.addObject(objectName.c_str(), boar);
	}

	// setInputMap() resets the controller's action edges, so the new hero
	// sees actions that are already held as pressed.
	_playerController.setInputMap(&game.getInputMap());
	Character::bindPlayerActions(_playerController);
	_playableCharacter->possess(&_playerController);

	_traversalMechanics.initialize(_levelManager.getTriggerDescriptors(),
									spawnPoint,
									kTraversalDefaultTimeLimitSeconds);
	_traversalMechanics.setTrackedCharacter(_playableCharacter);
	_traversalMechanics.setFrameDeltaSeconds(0.0f);
	_traversalMechanics.setEnabled(true);
	if (!_traversalOperatorRegistered) {
		_objectManager.pushOperator(&_traversalMechanics);
		_traversalOperatorRegistered = true;
	}

	_levelManager.attachCameraTo(_objectManager.getGameObject("Hero"), _objectManager, true, true);
	_initHUD();
}

bool PlayState::onExecute(float time)
{
#if _DEBUG
	if (DEBUGGING && _playableCharacter)
	{
		static float telemetryTimer = 0.0f;
		telemetryTimer += time;
		if (telemetryTimer >= 1.0f) {
			telemetryTimer = 0.0f;

			char buffer[192];
			sprintf_s(buffer, sizeof(buffer),
				"Run HUD: stamina=%.1f%% speed=%.1f run=%s\n",
				_playableCharacter->getStaminaNormalized() * 100.0f,
				_playableCharacter->getHorizontalSpeed(),
				_playableCharacter->isRunBoostActive() ? "ON" : "OFF");
			DEBUG_MSG(buffer);
		}
	}
#endif

	Keyboard* keyboard = Engine2D::getInput()->getKeyboard();
	const bool reloadDown = keyboard->down(Key::F5);
	const bool reloadPressed = reloadDown && !_reloadWasDown;
	_reloadWasDown = reloadDown;
	if (Debug::Mode.isEnabled() && reloadPressed) {
		// Shift+F5 respawns at the map-authored spawn point; plain F5 keeps the
		// hero where it stands so map edits can be checked in place.
		const bool respawn = keyboard->down(Key::LeftShift) ||
			keyboard->down(Key::RightShift);
		const bool keepPosition = !respawn && _playableCharacter;
		const vector2 heroPosition = keepPosition ? _playableCharacter->getPosition() : vector2();

		// Drain queued events while their senders are still alive, then rebuild
		// the stage through its normal lifecycle to reread map and tileset data.
		Engine2D::getEventSystem()->processEvents();
		onExit(nullptr);
		_paused = false;
		onEnter(nullptr);
		// Object-added events start the new objects and register their renderables.
		Engine2D::getEventSystem()->processEvents();
		if (keepPosition && _playableCharacter) {
			_playableCharacter->setPosition(heroPosition);
		}
		DEBUG_MSG(keepPosition ? "Stage reloaded from disk in place.\n" :
			"Stage reloaded from disk at spawn point.\n");
	}
	const InputMap& input = game.getInputMap();

	// First frame after a pause: the pause overlay was popped and this state is
	// top again, so make the HUD cursor visible once more.
	if (_paused) {
		_paused = false;
		if (_cursor && _cursor->getImage()) {
			_cursor->getImage()->setVisibility(true);
		}
	}

	// First frame after the game-over prompt is popped: this state is top again.
	// If the player chose to retry, respawn into a fresh start and re-show the
	// HUD cursor (it was hidden when the game-over overlay was pushed).
	if (_gameOverState && _gameOverState->wasRetryRequested()) {
		_gameOverState->consumeRetryRequest();
		_respawnPlayer();
		if (_cursor && _cursor->getImage()) {
			_cursor->getImage()->setVisibility(true);
		}
	}

	if (keyboard->pressed(Key::R))
	{
		_respawnPlayer();
	}

	if (input.pressed("PAUSE")) {
		// Hide the HUD cursor before pushing the pause overlay: its screen-space
		// list stays registered (push does not call onExit), and its position is
		// frozen because onExecute stops running, so leaving it visible would
		// draw a stale duplicate beside the pause overlay's own cursor.
		if (_cursor && _cursor->getImage()) {
			_cursor->getImage()->setVisibility(false);
		}
		_paused = true;
		// Push PauseState on top to freeze game while keeping world visible.
		// Create it once (onEnter/onExit are re-entrant and clean up after
		// themselves), so repeated pause/resume cycles reuse the same object.
		if (!_pauseState) {
			_pauseState = std::make_unique<PauseState>();
		}
		Engine2D::getGame()->push(_pauseState.get());
	}

	if (DEBUGGING) 
	{
		Camera* camera = _levelManager.getCamera();
		if (camera) {
			if (keyboard->pressed(Key::KeypadAdd)) {
				camera->setZoom(camera->getZoom() + 0.1f);
			}

			if (keyboard->pressed(Key::Equals)) {
				camera->setZoom(1.0f);
			}

			if (keyboard->pressed(Key::KeypadSubtract)) {
				camera->setZoom(camera->getZoom() - 0.1f);
			}

			if (keyboard->pressed(Key::F2)) {
				const Camera::ZoomAnchorMode nextMode =
					(camera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter) ?
					Camera::ZoomAnchorMode::OriginLegacy :
					Camera::ZoomAnchorMode::TargetCenter;
				camera->setZoomAnchorMode(nextMode);

				char buffer[128]{ 0 };
				sprintf_s(buffer, sizeof(buffer), "Camera Zoom Anchor: %s\n",
					(nextMode == Camera::ZoomAnchorMode::TargetCenter) ? "TargetCenter" : "OriginLegacy");
				DEBUG_MSG(buffer);
			}
		}
		if (keyboard->pressed(Key::F3)) {
			Debug::dbgCollision = !Debug::dbgCollision;

			char buffer[128]{ 0 };
			sprintf_s(buffer, sizeof(buffer), "Collision Debug: %s\n",
				Debug::dbgCollision ? "ON" : "OFF");
			DEBUG_MSG(buffer);
		}
	}

	_traversalMechanics.setFrameDeltaSeconds(time);
	// This tick's actions reach the hero's state machine before the world
	// updates: boars read the hero's state, so the order matters.
	if (_playableCharacter) {
		_playerController.sendActionConditions(*_playableCharacter);
	}
	const bool keepRunning = GameState::onExecute(time);

	// A destination's next_map reloads the stage with the next map through the normal
	// lifecycle, the same way the F5 in-place reload does.
	{
		std::string nextMap;
		std::string entryName;
		float characterY = 0.0f;
		if (_traversalMechanics.consumeMapChangeRequest(nextMap, &entryName, &characterY) && !nextMap.empty()) {
			_sectionEntryName = entryName;
			_sectionEntryCharacterY = characterY;
			_sectionEntryFacingLeft = _playableCharacter && _playableCharacter->isFacingLeft();
			Engine2D::getEventSystem()->processEvents();
			onExit(nullptr);
			_paused = false;
			_mapFileName = nextMap;
			onEnter(nullptr);
			Engine2D::getEventSystem()->processEvents();
		}
	}

	// Killzones (e.g. deep water) send the player back to the last checkpoint. Run
	// timeouts still do not respawn, as before.
	vector2 traversalRespawn(0.0f, 0.0f);
	std::string respawnReason;
	if (_traversalMechanics.consumeRespawnRequest(traversalRespawn, &respawnReason) &&
		_playableCharacter && respawnReason != "timeout") {
		_collisionSystem.reset();
		_playableCharacter->clearEvents();
		_playableCharacter->resetForRespawn();
		_playableCharacter->setState(_playableCharacter->getState("Falling"));
		_playableCharacter->setPosition(traversalRespawn);
		_levelProps.resetPlatforms();
	}

	if (_playableCharacter) {
		// UP (directional, shared with the menus) and INTERACT (E / USE,
		// gameplay-only) both interact.
		const bool interactPressed = input.pressed("UP") || input.pressed("INTERACT");
		_levelProps.update(_playableCharacter, interactPressed, time);
	}

	for (Boar* boar : _boars) {
		if (boar) boar->updateCombat(time);
	}
	_levelManager.update();
	_updateHUD(time);

	// Detect death: the character is in its "Dead" state (via boar combat or a
	// DEATH command).  Let the death animation play in full before freezing the
	// world and showing the GAME OVER prompt on top; the dead character stays
	// visible behind it.  The "Dead" animation is one-shot (eOnce), so it stops
	// (isPlaying() -> false) the frame after its last frame has shown; that is
	// when we bring up the game-over screen.  (PlayState is only executed while
	// it is the top state, so this fires once per death and never again until a
	// retry respawns the character out of "Dead".)
	if (_playableCharacter &&
		_playableCharacter->getState() &&
		!strcmp(_playableCharacter->getState()->getName(), "Dead")) {
		Animation* deadAnimation = static_cast<Animation*>(_playableCharacter->getRenderable());
		if (!deadAnimation || !deadAnimation->isPlaying()) {
			if (_cursor && _cursor->getImage()) {
				_cursor->getImage()->setVisibility(false);
			}
			if (!_gameOverState) {
				_gameOverState = std::make_unique<GameOverState>();
			}
			Engine2D::getGame()->push(_gameOverState.get());
			return keepRunning;
		}
	}

	// The HUD cursor tracks the mouse in screen space (not through the camera).
	Mouse* mouse = Engine2D::getInput()->getMouse();
	if (!_paused && _cursor && mouse) {
		_cursor->follow(*mouse);
	}

	return keepRunning;
}

void PlayState::onExit(State* next)
{
	_shutdownHUD();
	_traversalMechanics.setEnabled(false);
	_traversalMechanics.setTrackedCharacter(NULL);
	_traversalMechanics.setFrameDeltaSeconds(0.0f);

	if (_playableCharacter) {
		_objectManager.removeObject(_playableCharacter);
	}

	for (Boar* boar : _boars) {
		if (boar) {
			_objectManager.removeObject(boar);
			delete boar;
		}
	}
	_boars.clear();
	SAFE_DELETE(_playableCharacter);
	
	_levelProps.clear();
 	_levelManager.shutdown(_objectManager, *this);

	GameState::onExit(next);
}
