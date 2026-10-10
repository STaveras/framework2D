// Verifies the whole-image (.tmj imagelayer) support end to end: the JSON parses into a
// TileImageLayerDescriptor with the right path/offset/parallax/repeat/tint, and a real
// image layer builds static repeat copies that land in the layer's render list.
#include "../src/Engine2D.h"
#include "../src/FantasySideScroller/LevelManager.h"
#include "../src/GameState.h"
#include "../src/System.h"
#include "../src/TileMap.h"
#include "../src/Camera.h"
#include "../src/Color.h"
#include "../src/Renderable.h"
#include "../src/Sprite.h"
#include "stb/stb_image.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <system_error>
#include <string>
#include <vector>

namespace {

class TestTexture : public ITexture {
	unsigned int width = 0, height = 0;
public:
	explicit TestTexture(const char* path) : ITexture(path) {
		int w = 0, h = 0, channels = 0;
		const bool loaded = stbi_info(path, &w, &h, &channels) != 0;
		assert(loaded);
		width = (unsigned int)w;
		height = (unsigned int)h;
	}
	unsigned int getWidth() const override { return width; }
	unsigned int getHeight() const override { return height; }
};

class TestRenderer : public IRenderer {
public:
	ITexture* createTexture(const char* path, Color = 0) override {
		auto* texture = new TestTexture(path);
		m_Textures.store(texture);
		return texture;
	}
	void initialize() override {}
	void shutdown() override {}
	void render() override {}
};

bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }

} // namespace

int main()
{
	std::error_code directoryError;
	std::filesystem::create_directories("tmp", directoryError);
	if (directoryError) {
		std::cerr << "failed to create test temp directory: " << directoryError.message() << '\n';
		return 1;
	}

	System::GlobalDataPath("bin/fantasySideScroller");
	TestRenderer renderer;
	Engine2D::setRenderer(&renderer);

	// ---- Parsing: TileMap turns an imagelayer into a descriptor -------------------
	// The image path is resolved against the map's own folder (like a tileset source);
	// a temp map in tmp/ therefore points at tmp/Background/Background.png.
	{
		const char* mapPath = "tmp/image_layer_parse_test.tmj";
		std::ofstream map(mapPath);
		map << R"({"width":4,"height":4,"tilewidth":16,"tileheight":16,"orientation":"orthogonal",
		    "parallaxoriginx":10,"parallaxoriginy":20,
		    "layers":[
		      {"id":900,"name":"Sky","type":"imagelayer","visible":true,
		       "image":"Background/Background.png","x":0,"y":0,
		       "offsetx":32,"offsety":-8,"parallaxx":0,"parallaxy":0,
		       "opacity":0.5,"tintcolor":"#FF0000","transparentcolor":"#00FF00",
		       "repeatx":true,"repeaty":false,"class":"sky"}
		    ]})";
		map.close();

		TileMapLoadResult result = TileMap::loadMapDataFromJSONFile(mapPath, nullptr, false);
		assert(result.imageLayers.size() == 1);
		const TileImageLayerDescriptor& layer = result.imageLayers[0];
		assert(layer.id == 900);
		assert(layer.name == "Sky");
		assert(layer.className == "sky");
		assert(layer.visible);
		assert(layer.imagePath == "tmp/Background/Background.png");
		assert(near(layer.offsetX, 32.0f) && near(layer.offsetY, -8.0f));
		assert(near(layer.parallaxX, 0.0f) && near(layer.parallaxY, 0.0f));
		assert(near(layer.opacity, 0.5f));
		assert(layer.tintColor == 0xFFFF0000u);
		assert(layer.transparentColor == 0xFF00FF00u);
		assert(layer.repeatX && !layer.repeatY);

		// The layer still registers a (render) slot in traversal order.
		assert(result.layers.size() == 1);
		assert(result.layers[0].type == "imagelayer");
		std::remove(mapPath);
	}

	// ---- Group combination: a group's parallax/offset multiply into the child ------
	{
		const char* mapPath = "tmp/image_layer_group_test.tmj";
		std::ofstream map(mapPath);
		map << R"({"width":4,"height":4,"tilewidth":16,"tileheight":16,"orientation":"orthogonal",
		    "layers":[
		      {"id":1,"name":"G","type":"group","visible":true,
		       "offsetx":100,"parallaxx":2,"parallaxy":0.5,"opacity":0.5,
		       "layers":[
		         {"id":901,"name":"Sky","type":"imagelayer","visible":true,
		          "image":"Background/Background.png","x":0,"y":0,
		          "offsetx":5,"parallaxx":0.5,"parallaxy":2,"opacity":0.5}
		       ]}
		    ]})";
		map.close();

		TileMapLoadResult result = TileMap::loadMapDataFromJSONFile(mapPath, nullptr, false);
		assert(result.imageLayers.size() == 1);
		const TileImageLayerDescriptor& layer = result.imageLayers[0];
		assert(near(layer.offsetX, 105.0f));
		assert(near(layer.parallaxX, 1.0f)); // 2 * 0.5
		assert(near(layer.parallaxY, 1.0f)); // 0.5 * 2
		assert(near(layer.opacity, 0.25f)); // 0.5 * 0.5
		std::remove(mapPath);
	}

	// ---- Build: real image layer becomes static copies in its render list ----------
	// The map lives next to the real Background.png so the resolved path exists.
	{
		// The map file lives in the data folder (bin/fantasySideScroller); initialize()
		// resolves it via BasePath, so pass just the file name.
		const char* mapPath = "bin/fantasySideScroller/image_layer_build_test.tmj";
		const char* mapFileName = "image_layer_build_test.tmj";
		std::ofstream map(mapPath);
		map << R"({"width":4,"height":4,"tilewidth":16,"tileheight":16,"orientation":"orthogonal",
		    "layers":[
		      {"id":900,"name":"Sky","type":"imagelayer","visible":true,
		       "image":"Background/Background.png","x":0,"y":0,
		       "parallaxx":0,"parallaxy":0,"repeatx":true,"repeaty":true}
		    ]})";
		map.close();

		GameState gameState;
		ObjectManager& objectManager = *gameState.getObjectManager();
		LevelManager levelManager;
		levelManager.initialize(mapFileName, vector2(0, 0), "Background/Background.png", objectManager, gameState);

		// The layer produced at least its base copy, all sharing one texture.
		const std::vector<Image*>& sprites = levelManager.getImageLayerSprites();
		assert(!sprites.empty());
		const ITexture* firstTexture = sprites.front()->getTexture();
		assert(firstTexture != NULL);
		for (Image* sprite : sprites) {
			assert(sprite->getTexture() == firstTexture);
		}

		// The map has only the image layer, so its render list is the default one --
		// the same list a runtime object (the hero) is routed into. A zoom-change
		// rebuild must drop only the image copies and leave that object in place
		// (an earlier clear() wiped the whole shared list, losing the hero/boars/tiles).
		IRenderer::RenderList* defaultList = gameState.getDefaultRenderList();
		assert(defaultList != NULL);
		Sprite* hero = new Sprite();
		defaultList->push_back(hero);

		// A zoom change rebuilds the copies (the only runtime re-layout trigger). The
		// rebuild recreates the copies (and their shared texture), so re-capture it.
		levelManager.getCamera()->setZoom(2.0f);
		levelManager.update();
		assert(!levelManager.getImageLayerSprites().empty());
		const ITexture* rebuiltTexture = levelManager.getImageLayerSprites().front()->getTexture();
		assert(rebuiltTexture != NULL);
		for (Image* sprite : levelManager.getImageLayerSprites()) {
			assert(sprite->getTexture() == rebuiltTexture);
		}

		// The runtime object survived the rebuild.
		bool heroAlive = false;
		for (Renderable* item : *defaultList) {
			if (item == (Renderable*)hero) { heroAlive = true; break; }
		}
		assert(heroAlive);
		defaultList->remove(hero);
		delete hero;
		std::remove(mapPath);
	}

	std::cout << "image layer test passed\n";
	return 0;
}
