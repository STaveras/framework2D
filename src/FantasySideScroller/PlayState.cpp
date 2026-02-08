// PlayState.cpp
#include "PlayState.h"

#include "../Camera.h"

#include "Constants.h"
#include "Character.h"

PlayState::PlayState()
    : _player(nullptr)
    , _playableCharacter(nullptr) {

}

PlayState::~PlayState() {
    // ensure cleanup if exit wasn't called
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	_player = Engine2D::getGame()->getPlayers()->create();

	// Preferred: map-declared tilesets from the .tmj file.
	_levelManager.initialize("mockup_tiles2.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, _renderList);
	//_levelManager.initialize("testMap_separate_layers.tmj", vector2(-60.0f, 0.0f), "Background/Background.png", _objectManager, _renderList);

	_playableCharacter = new Character;
	_playableCharacter->setPosition(START_POSITION);

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
	_player->setGameObject(_playableCharacter);

	_levelManager.attachCameraTo(_objectManager.getGameObject("Hero"), _objectManager, true, true);
}

bool PlayState::onExecute(float time)
{
#if _DEBUG
	if (DEBUGGING)
	{
		static Timer timer; timer.update();

		if (timer.getElapsedTime() >= 1.0f) {
			timer.reset();
			
			// Output a hashed to the console
			char buffer[256];
			sprintf_s(buffer, sizeof(buffer), "PlayState::onExecute() - %s, %f\n", Engine2D::getTimer()->getTimeStamp().c_str(), Engine2D::getTimer()->getElapsedTime());
			DEBUG_MSG(buffer);
		}
	}
#endif

	Keyboard* keyboard = Engine2D::getInput()->getKeyboard();

	// TODO: Handle enemy spawning, game rules, etc.

	if (keyboard->keyPressed(keyboard->getKeys().KBK_R))
	{
		_playableCharacter->clearEvents();
		_playableCharacter->setState(_playableCharacter->getState("Falling"));
		_playableCharacter->setPosition(_playableCharacter->getPosition().x, -120);

		//if (_playableCharacter->tile)
	}

	if (keyboard->keyPressed(keyboard->getKeys().KBK_ESCAPE)) {

		// TODO: Bring up a menu (i.e. push a 'MenuState')

		Engine2D::quit();
	}

	if (DEBUGGING) {
		Camera* camera = _levelManager.getCamera();
		if (!camera) {
			return GameState::onExecute(time);
		}

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

		if (keyboard->keyPressed(keyboard->getKeys().KBK_F3)) {
			Debug::dbgCollision = !Debug::dbgCollision;

			char buffer[128]{ 0 };
			sprintf_s(buffer, sizeof(buffer), "Collision Debug: %s\n",
				Debug::dbgCollision ? "ON" : "OFF");
			DEBUG_MSG(buffer);
		}
	}
	_levelManager.update();

	return GameState::onExecute(time);
}

void PlayState::onExit(State* next)
{
	_player->finish();

	_objectManager.removeObject(_playableCharacter);

	SAFE_DELETE(_playableCharacter);
	
	_levelManager.shutdown(_objectManager, _renderList);

	Engine2D::getGame()->getPlayers()->destroy(_player);

	GameState::onExit(next);
}
