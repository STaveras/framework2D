// LevelManager.h
#pragma once

#include "../FollowObjectOperator.h"
#include "../IRenderer.h"
#include "../Maths.h"
#include "../TileMap.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Camera;
class GameState;
class GameObject;
class ObjectManager;
class TileSet;
class Prop;

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
	bool hasResolvedTileVisual = false;
	std::string tileTexturePath;
	RECT tileSrcRect{ 0, 0, 0, 0 };
	std::unordered_map<std::string, std::string> tileProperties;
	std::unordered_map<std::string, std::string> objectPropertyMap;
	std::unordered_map<std::string, std::string> mergedPropertyMap;

	const std::string* findMergedProperty(const std::string& name) const {
		std::unordered_map<std::string, std::string>::const_iterator itr = mergedPropertyMap.find(name);
		if (itr == mergedPropertyMap.end()) {
			return NULL;
		}
		return &(itr->second);
	}

	const std::string* findObjectProperty(const std::string& name) const {
		std::unordered_map<std::string, std::string>::const_iterator itr = objectPropertyMap.find(name);
		if (itr == objectPropertyMap.end()) {
			return NULL;
		}
		return &(itr->second);
	}
};

// Will at some point become the scene manager or whatever, but for now just handles loading a tile map and providing access to the camera and spawn point.

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
	bool _hasLevelBounds = false;
	vector2 _levelBoundsMin;
	vector2 _levelBoundsMax;
	int _runtimeLayerIndex = -1;
	std::vector<LevelTriggerDescriptor> _triggerDescriptors;
	std::vector<Prop*> _props;
	std::vector<GameObject*> _imageLayerObjects;
	AttachObjectsOperator _cameraPlayerAttach;
	bool _cameraAttachOperatorRegistered = false;

	TileMapLoadResult loadMapDataIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, GameState& gameState, const vector2& mapOffset);
	void _spawnPropsFromTriggers(ObjectManager& objectManager, GameState& gameState);
	void _clearProps(ObjectManager& objectManager, GameState& gameState);
	void _clearImageLayers(ObjectManager& objectManager, GameState& gameState);
	void clearCachedMapMetadata(void);
	void refreshLevelBounds(const TileMapLoadResult& loadResult, const vector2& mapOffset);
	void clampCameraToLevelBounds(void);

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
