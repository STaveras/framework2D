// PlayState.cpp
#include "PlayState.h"

#include "../Camera.h"
#include "../Debug.h"
#include "../Font.h"
#include "../Sprite.h"

#include "Constants.h"
#include "Character.h"
#include "Boar.h"

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

	if (!_helloWorldText) {
		_helloWorldText = new Font();
		const std::string fontPath = BasePath("Font/monogram/bitmap/monogram-bitmap.json");
		if (_helloWorldText->loadFromJSON(fontPath)) {
			_helloWorldText->setText("HelloWorld\nHello World");
			_helloWorldText->setTint(0xFFFFFFFF);
			_helloWorldText->setScale(kHUDTextScale, kHUDTextScale);
			_helloWorldText->setOffset(vector2(0.0f, 0.0f));
			_helloWorldText->setVisibility(true);
			_hudRenderList->push_back(_helloWorldText);
		}
		else {
			DEBUG_MSG(("Failed to load bitmap font from: " + fontPath + "\n").c_str());
			SAFE_DELETE(_helloWorldText);
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
	_staminaBarBackground->setScale(kHUDBackgroundWidth, kHUDStaminaHeight);

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
    
    if (_helloWorldText) {
        _helloWorldText->setPosition(hudOrigin + vector2(0.0f, kHUDTextOffsetY));
    }
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
		if (_helloWorldText) {
			_hudRenderList->remove(_helloWorldText);
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
	SAFE_DELETE(_helloWorldText);
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	_player = Engine2D::getGame()->getPlayers()->create();

	// Preferred: map-declared tilesets from the .tmj file.
	//_levelManager.initialize("mockup_tiles2.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);
	_levelManager.initialize("mosswood_hollow.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);

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
	_boar = new Boar(_objectManager, *_playableCharacter, spawnPoint + vector2(140.0f, 10.0f));
	_objectManager.addObject("Boar", _boar);

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


	if (keyboard->keyPressed(keyboard->getKeys().KBK_R))
	{
		_collisionSystem.reset();
		if (_boar) _boar->reset();
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

	// vector2 traversalRespawn(0.0f, 0.0f);
	// if (_traversalMechanics.consumeRespawnRequest(traversalRespawn) && _playableCharacter) {
	// 	_collisionSystem.reset();
	// 	_playableCharacter->clearEvents();
	// 	_playableCharacter->resetForRespawn();
	// 	_playableCharacter->setState(_playableCharacter->getState("Falling"));
	// 	_playableCharacter->setPosition(traversalRespawn);
	// }

	if (_boar) _boar->updateCombat(time);
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

	if (_boar) _objectManager.removeObject(_boar);
	SAFE_DELETE(_boar);
	SAFE_DELETE(_playableCharacter);
	
	_levelManager.shutdown(_objectManager, *this);

	if (_player) {
		Engine2D::getGame()->getPlayers()->destroy(_player);
		_player = NULL;
	}

	GameState::onExit(next);
}
