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
}

PlayState::PlayState()
    : _player(nullptr)
    , _playableCharacter(nullptr) {}

PlayState::~PlayState() {
    // ensure cleanup if exit wasn't called
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
}

void PlayState::_updateHUD(float dt)
{
	(void)dt;

	if (!_staminaBarBackground || !_staminaBarFill) {
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

		if (IRenderer* renderer = Engine2D::getRenderer()) {
			renderer->destroyRenderList(_hudRenderList);
		}
		_hudRenderList = NULL;
	}

	SAFE_DELETE(_staminaBarBackground);
	SAFE_DELETE(_staminaBarFill);
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	_player = Engine2D::getGame()->getPlayers()->create();

	// Preferred: map-declared tilesets from the .tmj file.
	//_levelManager.initialize("mockup_tiles2.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);
	_levelManager.initialize("testMap.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);
	// _levelManager.initialize("testMap_separate_layers.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, *this);

	_playableCharacter = new Character;
	if (_levelManager.hasSpawnPoint()) {
		_playableCharacter->setPosition(_levelManager.getSpawnPoint());
	}
	else {
		_playableCharacter->setPosition(START_POSITION);
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
	_player->getController()->addAction(Action("ATTACK", keyboard->getKeys().KBK_V));
	_player->getController()->addAction(Action("RUN", keyboard->getKeys().KBK_LSHIFT));
	_player->setGameObject(_playableCharacter);

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

	const bool keepRunning = GameState::onExecute(time);
	_levelManager.update();
	_updateHUD(time);
	return keepRunning;
}

void PlayState::onExit(State* next)
{
	_shutdownHUD();

	_player->finish();

	_objectManager.removeObject(_playableCharacter);

	SAFE_DELETE(_playableCharacter);
	
	_levelManager.shutdown(_objectManager, *this);

	Engine2D::getGame()->getPlayers()->destroy(_player);

	GameState::onExit(next);
}
