// PlayState.cpp
#include "PlayState.h"

#include "../TileMap.h"
#include "../Camera.h"

#include "Resources.h"
#include "Constants.h"
#include "Character.h"

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

void PlayState::onEnter(State* prev)
{
	GameState::onEnter();

	_player = Engine2D::getGame()->getPlayers()->create();
	_camera = new Camera();

	_pixel = new Image(BasePath("pixel.bmp").c_str());

	_background = new Image(BasePath("Background/Background.png").c_str());
	_background->center();

#ifdef _DEBUG
	_background->setVisibility(false);
	Renderer::get()->setBackgroundStatic(false);
#endif
	_renderList->push_back(_background);

	_tileSet = TileSet::loadFromFile(BasePath("Assets/fantasyTiles.tsj").c_str());

	//_tileMap = TileMap::loadFromCSVFile(BASE_DIRECTORY"testMap.csv", _tileSet);
	//_tileMap = (*TileMap::loadFromJSONFile(BASE_DIRECTORY"fantasyTestMap.tmj", _tileSet).begin());
	_tileMaps = TileMap::loadFromJSONFile(BasePath("testMap.tmj").c_str(), _tileSet);

	for (unsigned int j = 0; j < _tileMaps.size(); j++)
	{
		TileMap* _tileMap = _tileMaps[j];

		for (unsigned int i = 0; i < (unsigned int)_tileMap->getTiles().size(); i++) {
			char buffer[32]{ 0 }; sprintf_s(buffer, 32, "t%u", i);
			_objectManager.addObject(buffer, _tileMap->getTiles()[i]);
		}

		_tileMap->setPosition(-60, 0);
		_tileMap->arrangeTiles();
	}

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
		// Output a hashed to the console
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "PlayState::onExecute() - %s, %f\n", Engine2D::getTimer()->getTimeStamp().c_str(), Engine2D::getTimer()->getElapsedTime());
		DEBUG_MSG(buffer);
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
				_objectManager.removeObject(tileMap);
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

	GameState::onExit();
}
