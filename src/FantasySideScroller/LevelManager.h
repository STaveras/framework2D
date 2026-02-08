// LevelManager.h
#pragma once

#include "../FollowObjectOperator.h"
#include "../IRenderer.h"
#include "../Maths.h"

#include <vector>

class Camera;
class GameObject;
class ObjectManager;
class TileMap;
class TileSet;

class LevelManager
{
	Camera* _camera = NULL;
	Image* _background = NULL;
	Image* _pixel = NULL;
	TileSet* _tileSet = NULL;

	std::vector<TileMap*> _tileMaps;
	AttachObjectsOperator _cameraPlayerAttach;
	bool _cameraAttachOperatorRegistered = false;

	std::vector<TileMap*> loadTileMapsIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, const vector2& mapOffset);

public:
	LevelManager(void);
	~LevelManager(void);

	void initialize(const char* mapFileName,
		const vector2& mapOffset,
		const char* backgroundFileName,
		ObjectManager& objectManager,
		IRenderer::RenderList* renderList);

	void attachCameraTo(GameObject* target, ObjectManager& objectManager, bool followHorizontally = true, bool followVertically = true);
	void update(void);

	void shutdown(ObjectManager& objectManager, IRenderer::RenderList* renderList);

	Camera* getCamera(void) const { return _camera; }
	Image* getBackground(void) const { return _background; }
};
