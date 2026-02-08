// LevelManager.cpp
#include "LevelManager.h"

#include "../Camera.h"
#include "../Engine2D.h"
#include "../ObjectManager.h"
#include "../Renderer.h"
#include "../TileMap.h"

#include "Resources.h"

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

LevelManager::LevelManager(void) {}

LevelManager::~LevelManager(void) {}

std::vector<TileMap*> LevelManager::loadTileMapsIntoObjectManager(const char* mapFileName, ObjectManager& objectManager, const vector2& mapOffset)
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

void LevelManager::initialize(const char* mapFileName,
	const vector2& mapOffset,
	const char* backgroundFileName,
	ObjectManager& objectManager,
	IRenderer::RenderList* renderList)
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

	if (renderList && _background) {
		renderList->push_back(_background);
	}

	if (_tileMaps.empty() && mapFileName && mapFileName[0] != '\0') {
		_tileMaps = loadTileMapsIntoObjectManager(mapFileName, objectManager, mapOffset);
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
	if (_background && _camera) {
		_background->setPosition(_camera->getPosition());
	}
}

void LevelManager::shutdown(ObjectManager& objectManager, IRenderer::RenderList* renderList)
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
			objectManager.removeObject(tile);
		}
		delete tileMap;
	}
	_tileMaps.clear();

	objectManager.removeObject(_camera);

	if (renderList && _background) {
		renderList->remove(_background);
	}

	if (Engine2D::getRenderer() && Engine2D::getRenderer()->getCamera() == _camera) {
		Engine2D::getRenderer()->setCamera(NULL);
	}

	SAFE_DELETE(_tileSet);
	SAFE_DELETE(_background);
	SAFE_DELETE(_pixel);
	SAFE_DELETE(_camera);
}
