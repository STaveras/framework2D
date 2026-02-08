// PlayState.h
#pragma once

#include "../GameState.h"

#include "../FollowObjectOperator.h"

class Character;
class TileMap;
class TileSet;

class PlayState : public GameState
{
	Player* _player = NULL;
	Camera* _camera = NULL;

	// (Probably should just go in GameState...?)
	Image* _background = NULL;
	Image* _pixel = NULL;

	TileSet* _tileSet = NULL;

	std::vector<TileMap*> _tileMaps;

	AttachObjectsOperator _cameraPlayerAttach;
	//RenderCollisionVolumeBoundsOperator

	Character* _playableCharacter = NULL;

private:
	std::vector<TileMap*> loadTileMapsIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, const vector2& mapOffset);

public:
	PlayState(void);
	virtual ~PlayState(void);

	void onEnter(State* prev);
	bool onExecute(float time);
	void onExit(State* next);
};
