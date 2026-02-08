// PlayState.cpp
#include "PlayState.h"

#include "../TileMap.h"
#include "../Camera.h"

#include "Resources.h"
#include "Constants.h"
#include "Character.h"

#include <cctype>
#include <string>

namespace {
std::string sanitizeLayerName(const std::string& value)
{
	if (value.empty()) {
		return "unnamed";
	}

	std::string sanitized;
	sanitized.reserve(value.size());
	for (char c : value) {
		if (std::isalnum((unsigned char)c)) {
			sanitized.push_back(c);
		}
		else {
			sanitized.push_back('_');
		}
	}
	return sanitized;
}
}

PlayState::PlayState()
    : _player(nullptr)
    , _camera(nullptr)
    , _background(nullptr)
    , _pixel(nullptr)
    , _tileSet(nullptr)
    , _playableCharacter(nullptr) {

}

PlayState::~PlayState() {
    // ensure cleanup if exit wasn't called
}

std::vector<TileMap*> PlayState::loadTileMapsIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, const vector2& mapOffset)
{
	std::vector<TileMap*> tileMaps = TileMap::loadFromJSONFile(BasePath(mapFileName).c_str(), nullptr, false);

	for (TileMap* tileMap : tileMaps) {
		if (!tileMap) {
			continue;
		}

		tileMap->setPosition(mapOffset);
		tileMap->arrangeTiles();

		const TileLayerConfig& layerConfig = tileMap->getLayerConfig();
		const std::string safeLayerName = sanitizeLayerName(layerConfig.name);
		const std::string layerPrefix = "layer_" + std::to_string(layerConfig.id) + "_" + safeLayerName;

		unsigned int tileIndex = 0;
		for (auto it = tileMap->getTiles().begin(); it != tileMap->getTiles().end(); ++it, ++tileIndex) {
			Tile* tile = *it;
			if (!tile || tile->getTileIndex() < 0) {
				continue;
			}

			std::string objectName = layerPrefix + "_tile_" + std::to_string(tileIndex);
			objectManager.addObject(objectName.c_str(), tile);
		}
	}

	return tileMaps;
}

void PlayState::onEnter(State* prev)
{
	GameState::onEnter(prev);

	_player = Engine2D::getGame()->getPlayers()->create();
	_camera = new Camera();
	_camera->setZoomAnchorMode(Camera::ZoomAnchorMode::TargetCenter);
	_camera->setSnapToPixelGrid(true);

	_pixel = new Image(BasePath("pixel.bmp").c_str());

	_background = new Image(BasePath("Background/Background.png").c_str());
	_background->center();

#ifdef _DEBUG
	_background->setVisibility(false);
	//Renderer::get()->setBackgroundStatic(false);
#endif
	_renderList->push_back(_background);

	// Preferred: map-declared tilesets from the .tmj file.
	// _tileMaps = loadTileMapsIntoObjectManager("mockup_tiles2.tmj", _objectManager, vector2(-60.0f, 0.0f));
	_tileMaps = loadTileMapsIntoObjectManager("testMap_separate_layers.tmj", _objectManager, vector2(-60.0f, 0.0f));

	_playableCharacter = new Character;
	_playableCharacter->setPosition(START_POSITION);

	_objectManager.addObject("Hero", _playableCharacter);
	_objectManager.addObject("Camera", _camera);

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

	_cameraPlayerAttach.setSource(_camera);
	_cameraPlayerAttach.follow(_objectManager.getGameObject("Hero"), true, true);
	_cameraPlayerAttach.setEnabled(true);

	_objectManager.pushOperator(&_cameraPlayerAttach);

	Engine2D::getRenderer()->setCamera(_camera);
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
		if (keyboard->keyPressed(keyboard->getKeys().KBK_ADD)) {
			_camera->setZoom(_camera->getZoom() + 0.1f);
		}

		if (keyboard->keyPressed(keyboard->getKeys().KBK_EQUALS)) {
			_camera->setZoom(1.0f);
		}

		if (keyboard->keyPressed(keyboard->getKeys().KBK_SUBTRACT)) {
			_camera->setZoom(_camera->getZoom() - 0.1f);
		}

		if (keyboard->keyPressed(keyboard->getKeys().KBK_F2)) {
			const Camera::ZoomAnchorMode nextMode =
				(_camera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter) ?
				Camera::ZoomAnchorMode::OriginLegacy :
				Camera::ZoomAnchorMode::TargetCenter;
			_camera->setZoomAnchorMode(nextMode);

			char buffer[128]{ 0 };
			sprintf_s(buffer, sizeof(buffer), "Camera Zoom Anchor: %s\n",
				(nextMode == Camera::ZoomAnchorMode::TargetCenter) ? "TargetCenter" : "OriginLegacy");
			DEBUG_MSG(buffer);
		}
	}
	_background->setPosition(_camera->getPosition());

	return GameState::onExecute(time);
}

void PlayState::onExit(State* next)
{
	_player->finish();

	_objectManager.removeObject("Camera");
	_objectManager.removeObject("Hero");

	SAFE_DELETE(_playableCharacter);

	if (_tileMaps.size())
	{
		for (unsigned int i = 0; i < _tileMaps.size(); i++)
		{
			TileMap* tileMap = _tileMaps[i];
			if (tileMap) {
				for (auto it = tileMap->getTiles().begin(); it != tileMap->getTiles().end(); ++it) {
					Tile* tile = *it;
					if (!tile || tile->getTileIndex() < 0) {
						continue;
					}
					_objectManager.removeObject(tile);
				}
				delete tileMap;
			}
		}
	}
	_tileMaps.clear();

	SAFE_DELETE(_tileSet);
	SAFE_DELETE(_background);
	SAFE_DELETE(_pixel);
	SAFE_DELETE(_camera);

	Engine2D::getGame()->getPlayers()->destroy(_player);

	GameState::onExit(next);
}
