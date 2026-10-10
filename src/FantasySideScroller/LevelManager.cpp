// LevelManager.cpp
#include "LevelManager.h"

#include "../Camera.h"
#include "../Engine2D.h"
#include "../GameState.h"
#include "../ObjectManager.h"
#include "../Renderer.h"
#include "../StrUtils.h"

#include "CharacterTuning.h"
#include "Resources.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <utility>

namespace {
bool isSpawnType(const std::string& typeName)
{
	return StrUtils::IEquals(typeName, "spawn");
}

bool isEnemyType(const std::string& typeName)
{
	return StrUtils::IEquals(typeName, "boar");
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
	_spawnPoints.clear();
	_hasLevelBounds = false;
	_levelBoundsMin = vector2(0.0f, 0.0f);
	_levelBoundsMax = vector2(0.0f, 0.0f);
	_runtimeLayerIndex = -1;
	_triggerDescriptors.clear();
	_enemyDescriptors.clear();
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

		// A layer's pixel offset only nudges its art; it does not make the level larger, so
		// the camera must not scroll past the map's own edges because of it.
		const TileLayerConfig& layerConfig = tileMap->getLayerConfig();
		const float left = mapOffset.x + ((float)layerConfig.startX * tileWidth);
		const float top = mapOffset.y + ((float)layerConfig.startY * tileHeight);
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

vector2 LevelManager::getViewportHalfSize(float zoom) const
{
	if (zoom <= 0.0f)
	{
		zoom = 1.0f;
	}

	int screenWidth = 0;
	int screenHeight = 0;
	if (_camera)
	{
		screenWidth = _camera->getScreenWidth();
		screenHeight = _camera->getScreenHeight();
	}
	if (screenWidth <= 0 || screenHeight <= 0)
	{
		IRenderer* renderer = Engine2D::getRenderer();
		if (renderer)
		{
			screenWidth = renderer->getWidth();
			screenHeight = renderer->getHeight();
		}
	}
	if (screenWidth <= 0 || screenHeight <= 0)
	{
		return vector2(0.0f, 0.0f);
	}

	// The view is centered on the parallax camera, so half the screen (over zoom) is
	// the reach in each direction. When the camera is rotated, the on-screen
	// rectangle's axis-aligned reach grows, exactly as the renderer's own view
	// bounds do, so cover the rotated corners too.
	const float angle = _camera ? _camera->getRotation() : 0.0f;
	const float c = std::abs(std::cos(angle));
	const float s = std::abs(std::sin(angle));
	return vector2(
		(((float)screenWidth * 0.5f) * c + ((float)screenHeight * 0.5f) * s) / zoom,
		(((float)screenWidth * 0.5f) * s + ((float)screenHeight * 0.5f) * c) / zoom);
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

void LevelManager::releaseImageLayerSprites(void)
{
	for (Image* sprite : _imageLayerSprites) {
		delete sprite;
	}
	_imageLayerSprites.clear();
	for (ImageLayerInfo& info : _imageLayerInfos) {
		info.sprites.clear();
	}
}

void LevelManager::destroyImageLayers(void)
{
	releaseImageLayerSprites();
	_imageLayerInfos.clear();
	_imageLayersBuilt = false;
	_imageLayerBuiltZoom = 0.0f;
}

// Lay out the whole-image layer sprites for the current zoom. Repeat axes tile
// enough static copies to cover the layer's parallax camera range plus half the
// view on each side; the existing chunk culling then drops whatever is off-screen.
void LevelManager::rebuildImageLayerCopies(void)
{
	const float zoom = _camera ? _camera->getZoom() : 1.0f;
	const vector2 halfSize = getViewportHalfSize(zoom);

	// Drop only this layer's own copies from the render lists before the sprites go
	// away, so the renderer never walks a dangling entry. The render list can be shared
	// with runtime objects (hero, boars, tiles) when it is the default list, so we must
	// not clear it wholesale -- only remove the sprites we added.
	for (ImageLayerInfo& info : _imageLayerInfos) {
		if (info.renderList) {
			for (Image* sprite : info.sprites) {
				if (sprite) {
					info.renderList->remove(sprite);
				}
			}
		}
	}
	releaseImageLayerSprites();

	if (_imageLayerInfos.empty()) {
		_imageLayersBuilt = true;
		_imageLayerBuiltZoom = zoom;
		return;
	}

	for (ImageLayerInfo& info : _imageLayerInfos) {
		const TileImageLayerDescriptor& descriptor = info.descriptor;

		if (!FileSystem::FileExists(descriptor.imagePath)) {
#if _DEBUG
			char buffer[512];
			sprintf_s(buffer, sizeof(buffer), "Image layer '%s' image not found: '%s'\n", descriptor.name.c_str(), descriptor.imagePath.c_str());
			DEBUG_MSG(buffer);
#endif
			continue;
		}

		Color clearColor;
		clearColor._color = descriptor.transparentColor;
		IRenderer* renderer = Engine2D::getRenderer();
		ITexture* texture = renderer ? renderer->createTexture(descriptor.imagePath.c_str(), clearColor) : NULL;
		if (!texture) {
			continue;
		}

		// Renderer textures are cached and renderer-owned. Keep layer sprites borrowed
		// so destroying a copy cannot invalidate another sprite using the same file.
		const RECT sourceRect = { 0, 0, (int)texture->getWidth(), (int)texture->getHeight() };
		Image* anchor = new Image(texture, sourceRect);
		if (!anchor || !anchor->getTexture()) {
			SAFE_DELETE(anchor);
#if _DEBUG
			char buffer[512];
			sprintf_s(buffer, sizeof(buffer), "Image layer '%s' failed to load '%s'\n", descriptor.name.c_str(), descriptor.imagePath.c_str());
			DEBUG_MSG(buffer);
#endif
			continue;
		}

		info.imageWidth = anchor->getWidth();
		info.imageHeight = anchor->getHeight();

		Color layerTint;
		layerTint._color = descriptor.tintColor;
		layerTint.a = (byte)(layerTint.a * std::max(0.0f, std::min(1.0f, descriptor.opacity)) + 0.5f);

		// The parallax camera sweeps this range as the map camera moves across the
		// level; widen it by half the view so the edge copies stay on screen.
		const vector2 camMin = vector2(
			info.parallaxOrigin.x + (_levelBoundsMin.x - info.parallaxOrigin.x) * descriptor.parallaxX,
			info.parallaxOrigin.y + (_levelBoundsMin.y - info.parallaxOrigin.y) * descriptor.parallaxY);
		const vector2 camMax = vector2(
			info.parallaxOrigin.x + (_levelBoundsMax.x - info.parallaxOrigin.x) * descriptor.parallaxX,
			info.parallaxOrigin.y + (_levelBoundsMax.y - info.parallaxOrigin.y) * descriptor.parallaxY);
		const float rangeMinX = camMin.x - halfSize.x;
		const float rangeMaxX = camMax.x + halfSize.x;
		const float rangeMinY = camMin.y - halfSize.y;
		const float rangeMaxY = camMax.y + halfSize.y;

		auto tilePositions = [](bool repeat, float base, float size, float lo, float hi) -> std::vector<float> {
			std::vector<float> positions;
			if (!repeat || size <= 0.0f) {
				positions.push_back(base);
				return positions;
			}
			// Copies sit at base + k*size; keep the ones that touch [lo, hi].
			const long long kMin = (long long)std::ceil((lo - size - base) / size);
			const long long kMax = (long long)std::floor((hi - base) / size);
			for (long long k = kMin; k <= kMax; ++k) {
				positions.push_back(base + (float)k * size);
			}
			return positions;
		};

		std::vector<float> xPositions = tilePositions(descriptor.repeatX, info.basePosition.x, info.imageWidth, rangeMinX, rangeMaxX);
		std::vector<float> yPositions = tilePositions(descriptor.repeatY, info.basePosition.y, info.imageHeight, rangeMinY, rangeMaxY);
		// The base copy must always exist so the layer renders even if the range came
		// out empty (e.g. a view not yet sized).
		if (xPositions.empty()) xPositions.push_back(info.basePosition.x);
		if (yPositions.empty()) yPositions.push_back(info.basePosition.y);

		IRenderer::RenderList* layerRenderList = info.renderList;
		bool isFirst = true;
		for (float x : xPositions) {
			for (float y : yPositions) {
				if (isFirst) {
					// The anchor owns the texture; the copies below share it.
					anchor->setPosition(x, y);
					anchor->setTint(layerTint);
					_imageLayerSprites.push_back(anchor);
					info.sprites.push_back(anchor);
					if (layerRenderList) {
						layerRenderList->push_back(anchor);
					}
					isFirst = false;
				} else {
					Image* sprite = new Image(*anchor);
					sprite->setPosition(x, y);
					sprite->setTint(layerTint);
					_imageLayerSprites.push_back(sprite);
					info.sprites.push_back(sprite);
					if (layerRenderList) {
						layerRenderList->push_back(sprite);
					}
				}
			}
		}
	}

	_imageLayersBuilt = true;
	_imageLayerBuiltZoom = zoom;
}

void LevelManager::buildImageLayers(const TileMapLoadResult& loadResult, const vector2& mapOffset)
{
	destroyImageLayers();

	for (const TileImageLayerDescriptor& descriptor : loadResult.imageLayers) {
		// Hidden layers and layers without an image cost nothing.
		if (!descriptor.visible || descriptor.imagePath.empty()) {
#if _DEBUG
			char buffer[512];
			sprintf_s(buffer, sizeof(buffer), "Skipping image layer '%s' (visible=%s image='%s')\n", descriptor.name.c_str(), descriptor.visible ? "true" : "false", descriptor.imagePath.c_str());
			DEBUG_MSG(buffer);
#endif
			continue;
		}

		ImageLayerInfo info;
		info.descriptor = descriptor;
		info.basePosition = mapOffset + vector2(descriptor.offsetX, descriptor.offsetY);
		info.parallaxOrigin = vector2(
			mapOffset.x + loadResult.parallaxOriginX,
			mapOffset.y + loadResult.parallaxOriginY);

		if (isValidLayerIndex(descriptor.traversalIndex, (int)_mapLayerRenderLists.size())) {
			info.renderList = _mapLayerRenderLists[(size_t)descriptor.traversalIndex];
		}
		else {
			info.renderList = NULL;
		}

		// Combine the layer's parallax with its render list.
		if (info.renderList) {
			info.renderList->parallaxX = descriptor.parallaxX;
			info.renderList->parallaxY = descriptor.parallaxY;
			info.renderList->parallaxOriginX = info.parallaxOrigin.x;
			info.renderList->parallaxOriginY = info.parallaxOrigin.y;
		}

		_imageLayerInfos.push_back(info);
	}

	rebuildImageLayerCopies();
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
		layerRenderList->parallaxX = layerConfig.parallaxX;
		layerRenderList->parallaxY = layerConfig.parallaxY;
		layerRenderList->parallaxOriginX = mapOffset.x + loadResult.parallaxOriginX;
		layerRenderList->parallaxOriginY = mapOffset.y + loadResult.parallaxOriginY;
		const std::string safeLayerName = StrUtils::SanitizeIdentifier(layerConfig.name);
		const std::string layerPrefix = "layer_" + std::to_string(layerConfig.id) + "_" + safeLayerName;

		unsigned int tileIndex = 0;
		for (auto it = tileMap->getTiles().begin(); it != tileMap->getTiles().end(); ++it, ++tileIndex) {
			Tile* tile = *it;
			if (!tile || tile->getTileIndex() < 0) {
				continue;
			}

			gameState.routeObjectToRenderList(tile, layerRenderList);
			if (Renderable* renderable = tile->getState() ? tile->getState()->getRenderable() : NULL) {
				// Tiled's layer tint and opacity multiply into every tile of the layer.
				if (layerConfig.tintColor != 0xFFFFFFFFu || layerConfig.opacity < 1.0f) {
					Color tint(layerConfig.tintColor);
					tint.a = (byte)(tint.a * std::max(0.0f, std::min(1.0f, layerConfig.opacity)) + 0.5f);
					renderable->setTint(tint);
				}
			}

			std::string objectName = layerPrefix + "_tile_" + std::to_string(tileIndex);
			objectManager.addObject(objectName.c_str(), tile);
			_registeredTiles.push_back(tile);
		}
	}

	refreshLevelBounds(loadResult, mapOffset);

	buildImageLayers(loadResult, mapOffset);

	for (const MapLayerDescriptor& layer : loadResult.layers) {
		if (layer.type != "objectgroup" || !isValidLayerIndex(layer.typedIndex, loadResult.objectLayers.size())) {
			continue;
		}

		const TileObjectLayerDescriptor& objectLayer = loadResult.objectLayers[(size_t)layer.typedIndex];
		for (const TileObjectDescriptor& object : objectLayer.objects) {
			const vector2 worldPosition = mapOffset + vector2(object.x, object.y);
			if (isSpawnType(object.typeName)) {
				// A spawn marker shows where the character's feet stand. Its origin sits
				// above them, so placing the origin on the marker would sink the body into
				// the ground and let collision shove it aside.
				const vector2 spawnPosition = worldPosition - vector2(0.0f, kLocomotionFootLocalY);
				_spawnPoints.push_back(spawnPosition);
				if (!_hasSpawnPoint) {
					_hasSpawnPoint = true;
					_spawnPoint = spawnPosition;
				}
				continue;
			}

			if (isEnemyType(object.typeName)) {
				LevelEnemyDescriptor enemy;
				enemy.objectId = object.id;
				enemy.typeName = object.typeName;
				enemy.position = worldPosition;
				_enemyDescriptors.push_back(std::move(enemy));
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
					_spawnPoints.push_back(_spawnPoint);
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

vector2 LevelManager::getSpawnPointNearDestination(const std::string& destinationName, float y) const
{
	const auto destination = std::find_if(_triggerDescriptors.begin(), _triggerDescriptors.end(),
		[&destinationName](const LevelTriggerDescriptor& descriptor) {
			return StrUtils::IEquals(descriptor.typeName, "destination") &&
				StrUtils::IEquals(descriptor.name, destinationName);
		});

	if (destination == _triggerDescriptors.end() || _spawnPoints.empty()) {
		return _spawnPoint;
	}

	// Enter beside the destination at the outgoing character's height. Comparing
	// only Y can select a spawn at the opposite end of the map.
	const vector2 entryPosition(destination->position.x + destination->size.x * 0.5f, y);
	const auto closestSpawn = std::min_element(_spawnPoints.begin(), _spawnPoints.end(),
		[entryPosition](const vector2& lhs, const vector2& rhs) {
			const vector2 lhsOffset = lhs - entryPosition;
			const vector2 rhsOffset = rhs - entryPosition;
			return dot(lhsOffset, lhsOffset) < dot(rhsOffset, rhsOffset);
		});
	return closestSpawn != _spawnPoints.end() ? *closestSpawn : _spawnPoint;
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

	// Repeat copies are laid out for one zoom; rebuild them if the zoom moved.
	if (_imageLayersBuilt && _camera && _camera->getZoom() != _imageLayerBuiltZoom) {
		rebuildImageLayerCopies();
	}
}

void LevelManager::shutdown(ObjectManager& objectManager, GameState& gameState)
{
	_cameraPlayerAttach.setEnabled(false);
	_cameraPlayerAttach.setSource(NULL);
	_cameraPlayerAttach.follow(NULL, true, true);

	// Drop the layer render lists before the tiles: the game state then forgets what it
	// placed in them, instead of searching a layer's list for each tile it removes.
	for (IRenderer::RenderList* renderList : _mapLayerRenderLists) {
		if (renderList) {
			Engine2D::getRenderer()->destroyRenderList(renderList);
		}
	}
	_mapLayerRenderLists.clear();

	// Image layers own their sprites; free them now that the render lists are gone.
	destroyImageLayers();

	gameState.setDefaultRenderList(gameState.getBaseRenderList());
	gameState.clearRenderRoutes();

	// Unregister exactly the tiles initialize() added. A tile's index can change
	// at runtime (collected keys are cleared to -1), so it can't decide this.
	objectManager.removeObjects(_registeredTiles);
	_registeredTiles.clear();

	for (TileMap* tileMap : _tileMaps)
	{
		delete tileMap;
	}
	_tileMaps.clear();
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
