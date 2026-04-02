// LevelManager.cpp
#include "LevelManager.h"

#include "../Camera.h"
#include "../Engine2D.h"
#include "../GameState.h"
#include "../ObjectManager.h"
#include "../Renderer.h"
#include "../StrUtils.h"

#include "Resources.h"

#include <algorithm>
#include <limits>
#include <string>
#include <utility>

namespace {
bool isSpawnType(const std::string& typeName)
{
	return StrUtils::IEquals(typeName, "spawn");
}

bool isValidLayerIndex(int index, size_t size)
{
	return index >= 0 && (size_t)index < size;
}

float clampValue(float value, float minValue, float maxValue)
{
	if (value < minValue) {
		return minValue;
	}
	if (value > maxValue) {
		return maxValue;
	}
	return value;
}
}

LevelManager::LevelManager(void) {}

LevelManager::~LevelManager(void) {}

void LevelManager::clearCachedMapMetadata(void)
{
	_hasSpawnPoint = false;
	_spawnPoint = vector2(0.0f, 0.0f);
	_hasLevelBounds = false;
	_levelBoundsMin = vector2(0.0f, 0.0f);
	_levelBoundsMax = vector2(0.0f, 0.0f);
	_runtimeLayerIndex = -1;
	_triggerDescriptors.clear();
}

void LevelManager::refreshLevelBounds(const TileMapLoadResult& loadResult, const vector2& mapOffset)
{
	_hasLevelBounds = false;
	_levelBoundsMin = vector2(0.0f, 0.0f);
	_levelBoundsMax = vector2(0.0f, 0.0f);

	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float maxY = std::numeric_limits<float>::lowest();

	auto includeRect = [&](float left, float top, float right, float bottom) {
		minX = std::min(minX, left);
		minY = std::min(minY, top);
		maxX = std::max(maxX, right);
		maxY = std::max(maxY, bottom);
		_hasLevelBounds = true;
	};

	for (TileMap* tileMap : loadResult.tileMaps) {
		if (!tileMap) {
			continue;
		}

		float tileWidth = tileMap->getTileSet() ? tileMap->getTileSet()->getTileSize() : 0.0f;
		float tileHeight = tileWidth;
		if (tileWidth <= 0.0f && loadResult.tileWidth > 0) {
			tileWidth = (float)loadResult.tileWidth;
		}
		if (tileHeight <= 0.0f && loadResult.tileHeight > 0) {
			tileHeight = (float)loadResult.tileHeight;
		}
		if (tileWidth <= 0.0f || tileHeight <= 0.0f) {
			continue;
		}

		const TileLayerConfig& layerConfig = tileMap->getLayerConfig();
		const float left = mapOffset.x + layerConfig.offsetX + ((float)layerConfig.startX * tileWidth);
		const float top = mapOffset.y + layerConfig.offsetY + ((float)layerConfig.startY * tileHeight);
		const float right = left + ((float)tileMap->getMapWidth() * tileWidth);
		const float bottom = top + ((float)tileMap->getMapHeight() * tileHeight);

		includeRect(
			std::min(left, right),
			std::min(top, bottom),
			std::max(left, right),
			std::max(top, bottom));
	}

	if (!_hasLevelBounds && loadResult.mapWidth > 0 && loadResult.mapHeight > 0) {
		float tileWidth = (float)((loadResult.tileWidth > 0) ? loadResult.tileWidth : loadResult.tileHeight);
		float tileHeight = (float)((loadResult.tileHeight > 0) ? loadResult.tileHeight : loadResult.tileWidth);
		if (tileWidth > 0.0f && tileHeight > 0.0f) {
			const float left = mapOffset.x;
			const float top = mapOffset.y;
			const float right = left + ((float)loadResult.mapWidth * tileWidth);
			const float bottom = top + ((float)loadResult.mapHeight * tileHeight);

			includeRect(
				std::min(left, right),
				std::min(top, bottom),
				std::max(left, right),
				std::max(top, bottom));
		}
	}

	if (_hasLevelBounds) {
		_levelBoundsMin = vector2(minX, minY);
		_levelBoundsMax = vector2(maxX, maxY);
	}
}

void LevelManager::clampCameraToLevelBounds(void)
{
	if (!_camera || !_hasLevelBounds) {
		return;
	}

	const float zoom = (_camera->getZoom() > 0.0f) ? _camera->getZoom() : 1.0f;
	const float halfScreenWidth = ((float)_camera->getScreenWidth() * 0.5f) / zoom;
	const float halfScreenHeight = ((float)_camera->getScreenHeight() * 0.5f) / zoom;

	const float minCameraX = _levelBoundsMin.x + halfScreenWidth;
	const float maxCameraX = _levelBoundsMax.x - halfScreenWidth;
	const float minCameraY = _levelBoundsMin.y + halfScreenHeight;
	const float maxCameraY = _levelBoundsMax.y - halfScreenHeight;

	vector2 cameraPosition = _camera->getPosition();
	if (minCameraX <= maxCameraX) {
		cameraPosition.x = clampValue(cameraPosition.x, minCameraX, maxCameraX);
	}
	else {
		cameraPosition.x = (_levelBoundsMin.x + _levelBoundsMax.x) * 0.5f;
	}

	if (minCameraY <= maxCameraY) {
		cameraPosition.y = clampValue(cameraPosition.y, minCameraY, maxCameraY);
	}
	else {
		cameraPosition.y = (_levelBoundsMin.y + _levelBoundsMax.y) * 0.5f;
	}

	_camera->setPosition(cameraPosition);
}

TileMapLoadResult LevelManager::loadMapDataIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, GameState& gameState, const vector2& mapOffset)
{
	TileSet::resetCollisionLoadStats();
	TileMapLoadResult loadResult = TileMap::loadMapDataFromJSONFile(BasePath(mapFileName).c_str(), nullptr, false);
	IRenderer* renderer = Engine2D::getRenderer();
	if (!renderer) {
		return loadResult;
	}

	for (size_t i = 0; i < loadResult.layers.size(); ++i) {
		_mapLayerRenderLists.push_back(renderer->createRenderList());
	}

	_runtimeLayerIndex = -1;
	for (const MapLayerDescriptor& layer : loadResult.layers) {
		if (layer.type == "objectgroup") {
			_runtimeLayerIndex = layer.traversalIndex;
			break;
		}
	}
	if (_runtimeLayerIndex < 0 && !_mapLayerRenderLists.empty()) {
		_runtimeLayerIndex = (int)_mapLayerRenderLists.size() - 1;
	}

	IRenderer::RenderList* runtimeRenderList = gameState.getBaseRenderList();
	if (isValidLayerIndex(_runtimeLayerIndex, _mapLayerRenderLists.size())) {
		runtimeRenderList = _mapLayerRenderLists[(size_t)_runtimeLayerIndex];
	}
	gameState.setDefaultRenderList(runtimeRenderList);

	for (const MapLayerDescriptor& layer : loadResult.layers) {
		if (layer.type != "tilelayer" || !isValidLayerIndex(layer.typedIndex, loadResult.tileMaps.size())) {
			continue;
		}

		TileMap* tileMap = loadResult.tileMaps[(size_t)layer.typedIndex];
		if (!tileMap) {
			continue;
		}

		_tileMaps.push_back(tileMap);
		tileMap->setPosition(mapOffset);
		tileMap->arrangeTiles();

		IRenderer::RenderList* layerRenderList = gameState.getBaseRenderList();
		if (isValidLayerIndex(layer.traversalIndex, _mapLayerRenderLists.size())) {
			layerRenderList = _mapLayerRenderLists[(size_t)layer.traversalIndex];
		}

		const TileLayerConfig& layerConfig = tileMap->getLayerConfig();
		const std::string safeLayerName = StrUtils::SanitizeIdentifier(layerConfig.name);
		const std::string layerPrefix = "layer_" + std::to_string(layerConfig.id) + "_" + safeLayerName;

		unsigned int tileIndex = 0;
		for (auto it = tileMap->getTiles().begin(); it != tileMap->getTiles().end(); ++it, ++tileIndex) {
			Tile* tile = *it;
			if (!tile || tile->getTileIndex() < 0) {
				continue;
			}

			gameState.routeObjectToRenderList(tile, layerRenderList);

			std::string objectName = layerPrefix + "_tile_" + std::to_string(tileIndex);
			objectManager.addObject(objectName.c_str(), tile);
		}
	}

	refreshLevelBounds(loadResult, mapOffset);

	for (const MapLayerDescriptor& layer : loadResult.layers) {
		if (layer.type != "objectgroup" || !isValidLayerIndex(layer.typedIndex, loadResult.objectLayers.size())) {
			continue;
		}

		const TileObjectLayerDescriptor& objectLayer = loadResult.objectLayers[(size_t)layer.typedIndex];
		for (const TileObjectDescriptor& object : objectLayer.objects) {
			const vector2 worldPosition = mapOffset + vector2(object.x, object.y);
			if (!_hasSpawnPoint && isSpawnType(object.typeName)) {
				_hasSpawnPoint = true;
				_spawnPoint = worldPosition;
				continue;
			}

			if (isSpawnType(object.typeName)) {
				continue;
			}

			LevelTriggerDescriptor trigger;
			trigger.layerId = object.layerId;
			trigger.layerName = object.layerName;
			trigger.objectId = object.id;
			trigger.name = object.name;
			trigger.typeName = object.typeName;
			trigger.gid = object.gid;
			trigger.position = worldPosition;
			trigger.size = vector2(object.width, object.height);
			trigger.rotation = object.rotation;
			trigger.visible = object.visible;
			trigger.isPoint = object.isPoint;
			trigger.isEllipse = object.isEllipse;
			trigger.hasPolygon = object.hasPolygon;
			trigger.hasPolyline = object.hasPolyline;
			trigger.polygonPoints = object.polygonPoints;
			trigger.polylinePoints = object.polylinePoints;
			trigger.properties.reserve(object.properties.size());
			for (const TileMapPropertyDescriptor& property : object.properties) {
				TriggerPropertyDescriptor triggerProperty;
				triggerProperty.name = property.name;
				triggerProperty.type = property.type;
				triggerProperty.value = property.value;
				trigger.properties.push_back(std::move(triggerProperty));
			}

			_triggerDescriptors.push_back(std::move(trigger));
		}
	}

	if (!_hasSpawnPoint) {
		for (const MapLayerDescriptor& layer : loadResult.layers) {
			if (layer.type != "tilelayer" || !isValidLayerIndex(layer.typedIndex, loadResult.tileMaps.size())) {
				continue;
			}

			TileMap* tileMap = loadResult.tileMaps[(size_t)layer.typedIndex];
			if (!tileMap) {
				continue;
			}

			bool foundSpawn = false;
			for (unsigned int y = 0; y < tileMap->getMapHeight() && !foundSpawn; ++y) {
				for (unsigned int x = 0; x < tileMap->getMapWidth() && !foundSpawn; ++x) {
					Tile* tile = tileMap->getTile(x, y);
					if (!tile || tile->getTileIndex() < 0) {
						continue;
					}
					if (!isSpawnType(tile->getTileType())) {
						continue;
					}

					const float tileSize = tile->getTileSet() ? tile->getTileSet()->getTileSize() : (float)loadResult.tileWidth;
					_spawnPoint = tile->getPosition() + vector2(tileSize * 0.5f, tileSize * 0.5f);
					_hasSpawnPoint = true;
					foundSpawn = true;
				}
			}

			if (_hasSpawnPoint) {
				break;
			}
		}
	}

	return loadResult;
}

void LevelManager::initialize(const char* mapFileName,
	const vector2& mapOffset,
	const char* backgroundFileName,
	ObjectManager& objectManager,
	GameState& gameState)
{
	if (!_camera) {
		_camera = new Camera();
		_camera->setZoomAnchorMode(Camera::ZoomAnchorMode::TargetCenter);
		_camera->setSnapToPixelGrid(true);
	}

	if (!_pixel) {
		_pixel = new Image(BasePath("pixel.bmp").c_str());
	}

	if (!_background && backgroundFileName && backgroundFileName[0] != '\0') {
		_background = new Image(BasePath(backgroundFileName).c_str());
		_background->center();
#ifdef _DEBUG
		_background->setVisibility(false);
#endif
	}

	if (IRenderer::RenderList* baseRenderList = gameState.getBaseRenderList()) {
		if (_background && std::find(baseRenderList->begin(), baseRenderList->end(), _background) == baseRenderList->end()) {
			baseRenderList->push_back(_background);
		}
	}

	if (_tileMaps.empty() && mapFileName && mapFileName[0] != '\0') {
		clearCachedMapMetadata();
		(void)loadMapDataIntoObjectManager(mapFileName, objectManager, gameState, mapOffset);
	}
	else {
		IRenderer::RenderList* runtimeRenderList = gameState.getBaseRenderList();
		if (isValidLayerIndex(_runtimeLayerIndex, _mapLayerRenderLists.size())) {
			runtimeRenderList = _mapLayerRenderLists[(size_t)_runtimeLayerIndex];
		}
		gameState.setDefaultRenderList(runtimeRenderList);
	}

	if (objectManager.getObjectName(_camera).empty()) {
		objectManager.addObject("Camera", _camera);
	}
	Engine2D::getRenderer()->setCamera(_camera);
}

void LevelManager::attachCameraTo(GameObject* target, ObjectManager& objectManager, bool followHorizontally, bool followVertically)
{
	if (!_camera || !target) {
		return;
	}

	_cameraPlayerAttach.setSource(_camera);
	_cameraPlayerAttach.follow(target, followHorizontally, followVertically);
	_cameraPlayerAttach.setEnabled(true);

	if (!_cameraAttachOperatorRegistered) {
		objectManager.pushOperator(&_cameraPlayerAttach);
		_cameraAttachOperatorRegistered = true;
	}
}

void LevelManager::update(void)
{
	clampCameraToLevelBounds();

	if (_background && _camera) {
		_background->setPosition(_camera->getPosition());
	}
}

void LevelManager::shutdown(ObjectManager& objectManager, GameState& gameState)
{
	_cameraPlayerAttach.setEnabled(false);
	_cameraPlayerAttach.setSource(NULL);
	_cameraPlayerAttach.follow(NULL, true, true);

	for (TileMap* tileMap : _tileMaps)
	{
		if (!tileMap) {
			continue;
		}

		for (auto it = tileMap->getTiles().begin(); it != tileMap->getTiles().end(); ++it) {
			Tile* tile = *it;
			if (!tile || tile->getTileIndex() < 0) {
				continue;
			}
			gameState.clearObjectRenderRoute(tile);
			objectManager.removeObject(tile);
		}
		delete tileMap;
	}
	_tileMaps.clear();

	for (IRenderer::RenderList* renderList : _mapLayerRenderLists) {
		if (renderList) {
			Engine2D::getRenderer()->destroyRenderList(renderList);
		}
	}
	_mapLayerRenderLists.clear();

	gameState.setDefaultRenderList(gameState.getBaseRenderList());
	gameState.clearRenderRoutes();
	clearCachedMapMetadata();

	objectManager.removeObject(_camera);

	if (IRenderer::RenderList* baseRenderList = gameState.getBaseRenderList()) {
		if (_background) {
			baseRenderList->remove(_background);
		}
	}

	if (Engine2D::getRenderer() && Engine2D::getRenderer()->getCamera() == _camera) {
		Engine2D::getRenderer()->setCamera(NULL);
	}

	SAFE_DELETE(_tileSet);
	SAFE_DELETE(_background);
	SAFE_DELETE(_pixel);
	SAFE_DELETE(_camera);
}
