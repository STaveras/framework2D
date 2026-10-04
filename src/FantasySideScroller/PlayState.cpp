// File: PlayState.cpp
// Author: Stanley Taveras
// Created: 2/18/2010
// Modified: 11/13/2024

#include "PlayState.h"

#include "../Camera.h"
#include "../Debug.h"
#include "../Font.h"
#include "../Gamepad.h"
#include "../Sprite.h"
#include "../Cursor.h"
#include "PauseState.h"
#include "ScreenSpaceCursor.h"

#include "Constants.h"
#include "Character.h"
#include "Boar.h"
#include "../StrUtils.h"

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
    : _player(nullptr)
    , _playableCharacter(nullptr)
    , _cursor(nullptr) {}

PlayState::~PlayState() {
	if (_player || _playableCharacter || _hudRenderList) {
		onExit(nullptr);
	}
	// We own the pause overlay we push on top of ourselves; ProgramStack
	// never deletes states, so release it here.
	SAFE_DELETE(_pauseState);
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
		_healthBarBackground = new Sprite(BasePath("pixel.bmp").c_str());
		_healthBarBackground->setTint(0xAA101018);
		_healthBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);
		_healthBarBackground->setOffset(vector2(0.0f, 0.0f));
		_healthBarBackground->setVisibility(true);
		_hudRenderList->push_back(_healthBarBackground);
	}

	if (!_healthBarFill) {
		_healthBarFill = new Sprite(BasePath("pixel.bmp").c_str());
		_healthBarFill->setTint(0xFF32D060);
		_healthBarFill->setScale(kHUDFillMaxWidth, kHUDFillHeight);
		_healthBarFill->setOffset(vector2(0.0f, 0.0f));
		_healthBarFill->setVisibility(true);
		_hudRenderList->push_back(_healthBarFill);
	}

	if (!_staminaBarBackground) {
		_staminaBarBackground = new Sprite(BasePath("pixel.bmp").c_str());
		_staminaBarBackground->setTint(0xAA101018);
		_staminaBarBackground->setScale(kHUDBackgroundWidth, kHUDStaminaHeight);
		_staminaBarBackground->setOffset(vector2(0.0f, 0.0f));
		_staminaBarBackground->setVisibility(true);
		_hudRenderList->push_back(_staminaBarBackground);
	}

	if (!_staminaBarFill) {
		_staminaBarFill = new Sprite(BasePath("pixel.bmp").c_str());
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
		_keyIcon = new Sprite(BasePath("Assets/Tiles.png").c_str(), 0, RECT{ 240, 320, 256, 336 });
		_keyIcon->setVisibility(false);
		_hudRenderList->push_back(_keyIcon);
	}

	if (!_cursor) {
		_cursor = new Cursor();
		if (_cursor->load(BasePath("cursors.png").c_str())) {
			_cursor->getImage()->setVisibility(true);
			_hudRenderList->push_back(_cursor->getImage());
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
	SAFE_DELETE(_cursor);
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	_player = Engine2D::getGame()->getPlayers()->create();

	// Preferred: map-declared tilesets from the .tmj file.
	_levelManager.initialize("old_mine_trail.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);

	_playableCharacter = new Character;
	vector2 spawnPoint = START_POSITION;
	if (_levelManager.hasSpawnPoint()) {
		spawnPoint = _levelManager.getSpawnPoint();
		_playableCharacter->setPosition(spawnPoint);
	}
	else {
		_playableCharacter->setPosition(spawnPoint);
	}

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

	IKeyboard* keyboard = Engine2D::getInput()->getKeyboard();

	// TODO: Save the keymappings to a file and load them here
	_player->start();
	_player->setController(_inputManager.createController());
	Controller* controller = _player->getController();
	controller->addAction(Action("JUMP", keyboard->getKeys().KBK_SPACE));
	controller->addAction(Action("JUMP", IGamepad::Button::A));
	controller->addAction(Action("LEFT", keyboard->getKeys().KBK_LEFT));
	controller->addAction(Action("LEFT", keyboard->getKeys().KBK_A));
	controller->addAction(Action("LEFT", IGamepad::Button::DpadLeft));
	Action leftStick("LEFT");
	leftStick.assignAxis(IGamepad::Axis::LeftX, -0.25f);
	controller->addAction(leftStick);
	controller->addAction(Action("RIGHT", keyboard->getKeys().KBK_RIGHT));
	controller->addAction(Action("RIGHT", keyboard->getKeys().KBK_D));
	controller->addAction(Action("RIGHT", IGamepad::Button::DpadRight));
	Action rightStick("RIGHT");
	rightStick.assignAxis(IGamepad::Axis::LeftX, 0.25f);
	controller->addAction(rightStick);
	controller->addAction(Action("DOWN", keyboard->getKeys().KBK_DOWN));
	controller->addAction(Action("DOWN", keyboard->getKeys().KBK_S));
	controller->addAction(Action("DOWN", IGamepad::Button::DpadDown));
	controller->addAction(Action("ATTACK", keyboard->getKeys().KBK_LCONTROL));
	controller->addAction(Action("ATTACK", keyboard->getKeys().KBK_Z));
	controller->addAction(Action("ATTACK", IGamepad::Button::X));
	controller->addAction(Action("RUN", keyboard->getKeys().KBK_LSHIFT));
	controller->addAction(Action("RUN", IGamepad::Button::LeftBumper));
	controller->addAction(Action("INTERACT", keyboard->getKeys().KBK_UP));
	controller->addAction(Action("INTERACT", keyboard->getKeys().KBK_W));
	controller->addAction(Action("INTERACT", keyboard->getKeys().KBK_E));
	controller->addAction(Action("INTERACT", IGamepad::Button::DpadUp));
	controller->addAction(Action("INTERACT", IGamepad::Button::LeftThumb));
	// controller->addAction(Action("INTERACT", Gamepad::Button::Y));
	controller->addAction(Action("PAUSE", keyboard->getKeys().KBK_ESCAPE));
	controller->addAction(Action("PAUSE", IGamepad::Button::Start));
	_player->setGameObject(_playableCharacter);

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

	IKeyboard* keyboard = Engine2D::getInput()->getKeyboard();
	const bool reloadDown = keyboard->keyDown(keyboard->getKeys().KBK_F5);
	const bool reloadPressed = reloadDown && !_reloadWasDown;
	_reloadWasDown = reloadDown;
	if (Debug::Mode.isEnabled() && reloadPressed) {
		// Shift+F5 respawns at the map-authored spawn point; plain F5 keeps the
		// hero where it stands so map edits can be checked in place.
		const bool respawn = keyboard->keyDown(keyboard->getKeys().KBK_LSHIFT) ||
			keyboard->keyDown(keyboard->getKeys().KBK_RSHIFT);
		const bool keepPosition = !respawn && _playableCharacter;
		const vector2 heroPosition = keepPosition ? _playableCharacter->getPosition() : vector2();

		// Drain queued events while their senders are still alive, then rebuild
		// the stage through its normal lifecycle to reread map and tileset data.
		Engine2D::getEventSystem()->processEvents();
		onExit(nullptr);
		_interactWasActive = false;
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
	Controller* controller = _player ? _player->getController() : NULL;

	// First frame after a pause: the pause overlay was popped and this state is
	// top again, so make the HUD cursor visible once more.
	if (_paused) {
		_paused = false;
		if (_cursor && _cursor->getImage()) {
			_cursor->getImage()->setVisibility(true);
		}
	}

	if (keyboard->keyPressed(keyboard->getKeys().KBK_R))
	{
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
			sprintf_s(buffer, sizeof(buffer), "Respawn: pos={%.2f,%.2f} state=%s\n", pos.x, pos.y,
				_playableCharacter->getState() ? _playableCharacter->getState()->getName() : "(null)");
			DEBUG_MSG(buffer);
		}
#endif
	}

	Action* pauseAction = controller ? controller->getAction("PAUSE") : NULL;
	if (controller && controller->buttonPressed(pauseAction)) {
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
			_pauseState = new PauseState();
		}
		_pauseState->setController(controller);
		Engine2D::getGame()->push(_pauseState);
	}

	if (DEBUGGING) 
	{
		Camera* camera = _levelManager.getCamera();
		if (camera) {
			if (keyboard->keyPressed(keyboard->getKeys().KBK_ADD)) {
				camera->setZoom(camera->getZoom() + 0.1f);
			}

			if (keyboard->keyPressed(keyboard->getKeys().KBK_EQUALS)) {
				camera->setZoom(1.0f);
			}

			if (keyboard->keyPressed(keyboard->getKeys().KBK_SUBTRACT)) {
				camera->setZoom(camera->getZoom() - 0.1f);
			}

			if (keyboard->keyPressed(keyboard->getKeys().KBK_F2)) {
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
		if (keyboard->keyPressed(keyboard->getKeys().KBK_F3)) {
			Debug::dbgCollision = !Debug::dbgCollision;

			char buffer[128]{ 0 };
			sprintf_s(buffer, sizeof(buffer), "Collision Debug: %s\n",
				Debug::dbgCollision ? "ON" : "OFF");
			DEBUG_MSG(buffer);
		}
	}

	_traversalMechanics.setFrameDeltaSeconds(time);
	const bool keepRunning = GameState::onExecute(time);

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
		// Edge-detect on the action state (not raw keys) so gamepads and input replays work too.
		Action* interactAction = controller ? controller->getAction("INTERACT") : NULL;
		const bool interactActive = interactAction && interactAction->isActive();
		const bool interactPressed = interactActive && !_interactWasActive;
		_interactWasActive = interactActive;
		_levelProps.update(_playableCharacter, interactPressed, time);
	}

	for (Boar* boar : _boars) {
		if (boar) boar->updateCombat(time);
	}
	_levelManager.update();
	_updateHUD(time);

	// Update cursor position and state to match mouse.
	// The cursor sprite is pushed into _hudRenderList (a screen-space render
	// list). Convert the mouse's client coordinates into the renderer's logical
	// screen coordinates; do not convert through the camera/world transform.
	if (!_paused && _cursor) {
		IMouse* mouse = Engine2D::getInput()->getMouse();
		if (mouse) {
			_cursor->setPosition(ClientToRenderCursorPosition(mouse->getPosition()));
			_cursor->updateFromMouse(mouse);
		}
	}

	return keepRunning;
}

void PlayState::onExit(State* next)
{
	_shutdownHUD();
	_traversalMechanics.setEnabled(false);
	_traversalMechanics.setTrackedCharacter(NULL);
	_traversalMechanics.setFrameDeltaSeconds(0.0f);

	if (_player) {
		_player->finish();
	}

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

	if (_player) {
		Engine2D::getGame()->getPlayers()->destroy(_player);
		_player = NULL;
	}

	GameState::onExit(next);
}
