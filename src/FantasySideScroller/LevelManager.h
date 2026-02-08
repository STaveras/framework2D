// LevelManager.h
#pragma once

#include "../FollowObjectOperator.h"
#include "../IRenderer.h"
#include "../Maths.h"
#include "../TileMap.h"

#include <cstdint>
#include <string>
#include <vector>

class Camera;
class GameState;
class GameObject;
class ObjectManager;
class TileSet;

struct LevelTriggerDescriptor
{
	int layerId = -1;
	std::string layerName;
	int objectId = -1;
	std::string name;
	std::string typeName;
	int64_t gid = 0;
	vector2 position;
	vector2 size;
	float rotation = 0.0f;
	bool visible = true;
	bool isPoint = false;
	bool isEllipse = false;
	bool hasPolygon = false;
	bool hasPolyline = false;
	std::vector<vector2> polygonPoints;
	std::vector<vector2> polylinePoints;
	std::vector<TileMapPropertyDescriptor> properties;
};

class LevelManager
{
	Camera* _camera = NULL;
	Image* _background = NULL;
	Image* _pixel = NULL;
	TileSet* _tileSet = NULL;

	std::vector<TileMap*> _tileMaps;
	std::vector<IRenderer::RenderList*> _mapLayerRenderLists;
	bool _hasSpawnPoint = false;
	vector2 _spawnPoint;
	int _runtimeLayerIndex = -1;
	std::vector<LevelTriggerDescriptor> _triggerDescriptors;
	AttachObjectsOperator _cameraPlayerAttach;
	bool _cameraAttachOperatorRegistered = false;

	TileMapLoadResult loadMapDataIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, GameState& gameState, const vector2& mapOffset);
	void clearCachedMapMetadata(void);

public:
	LevelManager(void);
	~LevelManager(void);

	void initialize(const char* mapFileName,
		const vector2& mapOffset,
		const char* backgroundFileName,
		ObjectManager& objectManager,
		GameState& gameState);

	void attachCameraTo(GameObject* target, ObjectManager& objectManager, bool followHorizontally = true, bool followVertically = true);
	void update(void);

	void shutdown(ObjectManager& objectManager, GameState& gameState);

	Camera* getCamera(void) const { return _camera; }
	Image* getBackground(void) const { return _background; }
	bool hasSpawnPoint() const { return _hasSpawnPoint; }
	vector2 getSpawnPoint() const { return _spawnPoint; }
	const std::vector<LevelTriggerDescriptor>& getTriggerDescriptors() const { return _triggerDescriptors; }
	int getRuntimeLayerIndex() const { return _runtimeLayerIndex; }
};
