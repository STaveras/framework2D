// LevelManager.h
#pragma once

#include "../FollowObjectOperator.h"
#include "../IRenderer.h"
#include "../Maths.h"
#include "../TileMap.h"
#include "../Trigger.h"

#include <cstdint>
#include <string>
#include <vector>

class Camera;
class GameState;
class GameObject;
class ObjectManager;
class TileSet;

struct LevelEnemyDescriptor
{
	int objectId = -1;
	std::string typeName;
	vector2 position;
};

class LevelManager
{
	struct ImageLayerInfo
	{
		TileImageLayerDescriptor descriptor;
		IRenderer::RenderList* renderList = NULL;
		vector2 basePosition;
		vector2 parallaxOrigin;
		float imageWidth = 0.0f;
		float imageHeight = 0.0f;
		// This layer's own sprites, in the same order as _imageLayerSprites for this layer.
		// Tracked separately so a rebuild can drop only the copies this layer owns from a
		// render list that may also hold runtime objects (hero, boars, tiles).
		std::vector<Image*> sprites;
	};

	Camera* _camera = NULL;
	Image* _background = NULL;
	Image* _pixel = NULL;
	TileSet* _tileSet = NULL;

	std::vector<TileMap*> _tileMaps;
	std::vector<GameObject*> _registeredTiles;
	std::vector<IRenderer::RenderList*> _mapLayerRenderLists;
	bool _hasSpawnPoint = false;
	vector2 _spawnPoint;
	bool _hasLevelBounds = false;
	vector2 _levelBoundsMin;
	vector2 _levelBoundsMax;
	int _runtimeLayerIndex = -1;
	std::vector<LevelTriggerDescriptor> _triggerDescriptors;
	std::vector<LevelEnemyDescriptor> _enemyDescriptors;
	AttachObjectsOperator _cameraPlayerAttach;
	bool _cameraAttachOperatorRegistered = false;

	// Whole-image layers (sky, backdrops, reference art). Art only; no collision.
	std::vector<ImageLayerInfo> _imageLayerInfos;
	std::vector<Image*> _imageLayerSprites;
	float _imageLayerBuiltZoom = 0.0f;
	bool _imageLayersBuilt = false;

	TileMapLoadResult loadMapDataIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, GameState& gameState, const vector2& mapOffset);
	void clearCachedMapMetadata(void);
	void refreshLevelBounds(const TileMapLoadResult& loadResult, const vector2& mapOffset);
	void clampCameraToLevelBounds(void);
	void buildImageLayers(const TileMapLoadResult& loadResult, const vector2& mapOffset);
	void rebuildImageLayerCopies(void);
	void destroyImageLayers(void);
	void releaseImageLayerSprites(void);
	vector2 getViewportHalfSize(float zoom) const;

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
	const std::vector<LevelEnemyDescriptor>& getEnemyDescriptors() const { return _enemyDescriptors; }
	int getRuntimeLayerIndex() const { return _runtimeLayerIndex; }
	const std::vector<TileMap*>& getTileMaps() const { return _tileMaps; }
	const std::vector<Image*>& getImageLayerSprites() const { return _imageLayerSprites; }
};
