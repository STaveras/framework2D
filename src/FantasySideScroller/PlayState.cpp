// PlayState.cpp
#include "PlayState.h"

#include "../Camera.h"
#include "../Sprite.h"

#include "Constants.h"
#include "Character.h"

namespace {
constexpr float kHUDPaddingX = 12.0f;
constexpr float kHUDPaddingY = 12.0f;
constexpr float kHUDBackgroundWidth = 104.0f;
constexpr float kHUDBackgroundHeight = 10.0f;
constexpr float kHUDFillInset = 2.0f;
constexpr float kHUDFillMaxWidth = 100.0f;
constexpr float kHUDFillHeight = 6.0f;
constexpr float kHUDTimerOffsetY = 14.0f;
constexpr float kTraversalDefaultTimeLimitSeconds = 75.0f;
}

PlayState::PlayState()
    : _player(nullptr)
    , _playableCharacter(nullptr) {}

PlayState::~PlayState() {
	if (_player || _playableCharacter || _hudRenderList) {
		onExit(nullptr);
	}
}

void PlayState::_initHUD()
{
	IRenderer* renderer = Engine2D::getRenderer();
	if (!renderer) {
		return;
	}

	if (!_hudRenderList) {
		_hudRenderList = renderer->createRenderList();
	}

	if (!_staminaBarBackground) {
		_staminaBarBackground = new Image(BasePath("pixel.bmp").c_str());
		_staminaBarBackground->setTint(0xAA101018);
		_staminaBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);
		_staminaBarBackground->setOffset(vector2(0.0f, 0.0f));
		_staminaBarBackground->setVisibility(true);
		_hudRenderList->push_back(_staminaBarBackground);
	}

	if (!_staminaBarFill) {
		_staminaBarFill = new Image(BasePath("pixel.bmp").c_str());
		_staminaBarFill->setTint(0xFF32D060);
		_staminaBarFill->setScale(kHUDFillMaxWidth, kHUDFillHeight);
		_staminaBarFill->setOffset(vector2(0.0f, 0.0f));
		_staminaBarFill->setVisibility(true);
		_hudRenderList->push_back(_staminaBarFill);
	}

	if (!_timerBarBackground) {
		_timerBarBackground = new Image(BasePath("pixel.bmp").c_str());
		_timerBarBackground->setTint(0xAA101018);
		_timerBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);
		_timerBarBackground->setOffset(vector2(0.0f, 0.0f));
		_timerBarBackground->setVisibility(true);
		_hudRenderList->push_back(_timerBarBackground);
	}

	if (!_timerBarFill) {
		_timerBarFill = new Image(BasePath("pixel.bmp").c_str());
		_timerBarFill->setTint(0xFFD0A020);
		_timerBarFill->setScale(kHUDFillMaxWidth, kHUDFillHeight);
		_timerBarFill->setOffset(vector2(0.0f, 0.0f));
		_timerBarFill->setVisibility(true);
		_hudRenderList->push_back(_timerBarFill);
	}
}

void PlayState::_updateHUD(float dt)
{
	(void)dt;

	if (!_staminaBarBackground || !_staminaBarFill || !_timerBarBackground || !_timerBarFill) {
		return;
	}

	Camera* camera = _levelManager.getCamera();
	vector2 cameraPosition(0.0f, 0.0f);
	vector2 cameraCenter(0.0f, 0.0f);
	if (camera) {
		cameraPosition = camera->getRenderPosition();
		cameraCenter = camera->getCenter();
	}

	const vector2 cameraTopLeft = cameraPosition - cameraCenter;
	const vector2 hudOrigin = cameraTopLeft + vector2(kHUDPaddingX, kHUDPaddingY);

	_staminaBarBackground->setPosition(hudOrigin);
	_staminaBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);

	const float normalizedStamina = _playableCharacter ? _playableCharacter->getStaminaNormalized() : 0.0f;
	float clampedStamina = normalizedStamina;
	if (clampedStamina < 0.0f) {
		clampedStamina = 0.0f;
	}
	else if (clampedStamina > 1.0f) {
		clampedStamina = 1.0f;
	}

	const float fillWidth = kHUDFillMaxWidth * clampedStamina;
	const vector2 fillOrigin = hudOrigin + vector2(kHUDFillInset, kHUDFillInset);
	_staminaBarFill->setPosition(fillOrigin);
	_staminaBarFill->setScale(fillWidth, kHUDFillHeight);
	_staminaBarFill->setVisibility(fillWidth > 0.0f);

	const TraversalRunState& runState = _traversalMechanics.getRunState();
	const vector2 timerOrigin = hudOrigin + vector2(0.0f, kHUDTimerOffsetY);
	_timerBarBackground->setPosition(timerOrigin);
	_timerBarBackground->setScale(kHUDBackgroundWidth, kHUDBackgroundHeight);

	float timeRatio = 0.0f;
	if (runState.timeLimitSeconds > 0.0f) {
		timeRatio = runState.remainingSeconds / runState.timeLimitSeconds;
	}
	if (timeRatio < 0.0f) {
		timeRatio = 0.0f;
	}
	else if (timeRatio > 1.0f) {
		timeRatio = 1.0f;
	}

	const float timerFillWidth = kHUDFillMaxWidth * timeRatio;
	const vector2 timerFillOrigin = timerOrigin + vector2(kHUDFillInset, kHUDFillInset);
	_timerBarFill->setPosition(timerFillOrigin);
	_timerBarFill->setScale(timerFillWidth, kHUDFillHeight);
	_timerBarFill->setVisibility(timerFillWidth > 0.0f && !runState.completed);
}

void PlayState::_shutdownHUD()
{
	if (_hudRenderList) {
		if (_staminaBarBackground) {
			_hudRenderList->remove(_staminaBarBackground);
		}
		if (_staminaBarFill) {
			_hudRenderList->remove(_staminaBarFill);
		}
		if (_timerBarBackground) {
			_hudRenderList->remove(_timerBarBackground);
		}
		if (_timerBarFill) {
			_hudRenderList->remove(_timerBarFill);
		}

		if (IRenderer* renderer = Engine2D::getRenderer()) {
			renderer->destroyRenderList(_hudRenderList);
		}
		_hudRenderList = NULL;
	}

	SAFE_DELETE(_staminaBarBackground);
	SAFE_DELETE(_staminaBarFill);
	SAFE_DELETE(_timerBarBackground);
	SAFE_DELETE(_timerBarFill);
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	_player = Engine2D::getGame()->getPlayers()->create();

	// Preferred: map-declared tilesets from the .tmj file.
	_levelManager.initialize("mockup_tiles2.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);
	//_levelManager.initialize("testMap_separate_layers.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);

	_playableCharacter = new Character;
	vector2 spawnPoint = START_POSITION;
	if (_levelManager.hasSpawnPoint()) {
		spawnPoint = _levelManager.getSpawnPoint();
		_playableCharacter->setPosition(spawnPoint);
	}
	else {
		_playableCharacter->setPosition(spawnPoint);
	}

	_objectManager.addObject("Hero", _playableCharacter);

	Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

	// TODO: Save the keymappings to a file and load them here
	_player->start();
	_player->setController(_inputManager.createController());
	_player->getController()->addAction(Action("JUMP", keyboard->getKeys().KBK_SPACE));
	_player->getController()->addAction(Action("LEFT", keyboard->getKeys().KBK_LEFT));
	_player->getController()->addAction(Action("LEFT", keyboard->getKeys().KBK_A));
	_player->getController()->addAction(Action("RIGHT", keyboard->getKeys().KBK_RIGHT));
	_player->getController()->addAction(Action("RIGHT", keyboard->getKeys().KBK_D));
	_player->getController()->addAction(Action("DOWN", keyboard->getKeys().KBK_DOWN));
	_player->getController()->addAction(Action("DOWN", keyboard->getKeys().KBK_S));
	_player->getController()->addAction(Action("ATTACK", keyboard->getKeys().KBK_LCONTROL));
	_player->getController()->addAction(Action("RUN", keyboard->getKeys().KBK_LSHIFT));
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

	Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

	// TODO: Handle enemy spawning, game rules, etc.

	if (keyboard->keyPressed(keyboard->getKeys().KBK_R))
	{
		_collisionSystem.reset();
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

	if (keyboard->keyPressed(keyboard->getKeys().KBK_ESCAPE)) {

		// TODO: Bring up a menu (i.e. push a 'MenuState')

		Engine2D::quit();
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

	vector2 traversalRespawn(0.0f, 0.0f);
	if (_traversalMechanics.consumeRespawnRequest(traversalRespawn) && _playableCharacter) {
		_collisionSystem.reset();
		_playableCharacter->clearEvents();
		_playableCharacter->resetForRespawn();
		_playableCharacter->setState(_playableCharacter->getState("Falling"));
		_playableCharacter->setPosition(traversalRespawn);
	}

	_levelManager.update();
	_updateHUD(time);
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

	SAFE_DELETE(_playableCharacter);
	
	_levelManager.shutdown(_objectManager, *this);

	if (_player) {
		Engine2D::getGame()->getPlayers()->destroy(_player);
		_player = NULL;
	}

	GameState::onExit(next);
}
