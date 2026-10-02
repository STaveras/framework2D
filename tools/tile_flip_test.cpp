// Checks Tiled flip/rotation flags end to end: the gid bits survive map loading, the
// tile image transform lands every texel where Tiled draws it, and colliders flip too.
#include "../src/Engine2D.h"
#include "../src/Polygon.h"
#include "../src/Square.h"
#include "../src/System.h"
#include "../src/TileMap.h"
#include "stb/stb_image.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

class TestTexture : public ITexture {
    unsigned int width = 0, height = 0;
public:
    explicit TestTexture(const char* path) : ITexture(path) {
        int w = 0, h = 0, channels = 0;
        const bool loaded = stbi_info(path, &w, &h, &channels) != 0;
        assert(loaded);
        width = w;
        height = h;
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

constexpr float kSize = 16.0f;
constexpr unsigned int H = TileSet::kFlipHorizontal;
constexpr unsigned int V = TileSet::kFlipVertical;
constexpr unsigned int D = TileSet::kFlipDiagonal;

bool near(vector2 a, vector2 b) { return std::fabs(a.x - b.x) < 0.001f && std::fabs(a.y - b.y) < 0.001f; }

// Same corner math as RendererGL::_drawImage, relative to the sprite position.
vector2 renderedCorner(const Renderable* image, vector2 corner)
{
    const vector2 center = image->getCenter(), scale = image->getScale();
    const float c = std::cos(image->getRotation()), s = std::sin(image->getRotation());
    const float x = (corner.x - center.x) * scale.x, y = (corner.y - center.y) * scale.y;
    return vector2(c * x - s * y, s * x + c * y);
}

float signedArea(const std::vector<vector2>& points)
{
    float area = 0.0f;
    for (size_t i = 0; i < points.size(); ++i) {
        const vector2& a = points[i];
        const vector2& b = points[(i + 1) % points.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return area * 0.5f;
}

} // namespace

int main()
{
    // Tiled's documented rotations: D|H turns a tile 90 degrees clockwise, H|V 180, D|V 270.
    assert(near(TileSet::flipTilePoint(vector2(0, 0), kSize, D | H), vector2(16, 0)));
    assert(near(TileSet::flipTilePoint(vector2(16, 0), kSize, D | H), vector2(16, 16)));
    assert(near(TileSet::flipTilePoint(vector2(0, 0), kSize, H | V), vector2(16, 16)));
    assert(near(TileSet::flipTilePoint(vector2(0, 0), kSize, D | V), vector2(0, 16)));
    assert(near(TileSet::flipTilePoint(vector2(4, 1), kSize, D), vector2(1, 4)));

    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);

    // Tile 0 has an off-centre square collider (x 2, y 10, 14x6); tile 30 a slope polygon.
    TileSet* tiles = TileSet::loadFromFile("bin/fantasySideScroller/Assets/fantasyTiles.tsj");
    assert(tiles && tiles->getTileSize() == kSize);

    const unsigned int allFlags[] = {0, H, V, D, H | V, D | H, D | V, D | H | V};
    for (unsigned int flags : allFlags) {
        Tile tile;
        tile.setTileSet(tiles);
        tile.setTileIndex(0, flags);
        assert(tile.getFlipFlags() == flags);

        // Every texel corner must render where Tiled's flip puts it inside the cell.
        const Renderable* image = tile.getState()->getRenderable();
        for (vector2 corner : {vector2(0, 0), vector2(16, 0), vector2(16, 16), vector2(0, 16), vector2(3, 7)}) {
            assert(near(renderedCorner(image, corner), TileSet::flipTilePoint(corner, kSize, flags)));
        }

        const Square* square = (const Square*)tile.getState()->getCollidable();
        assert(square && square->getType() == COL_OBJ_SQUARE);
        const vector2 a = TileSet::flipTilePoint(vector2(2, 10), kSize, flags);
        const vector2 b = TileSet::flipTilePoint(vector2(16, 16), kSize, flags);
        assert(near(square->getPosition(), vector2(std::fmin(a.x, b.x), std::fmin(a.y, b.y))));
        assert(std::fabs(square->getWidth() - std::fabs(b.x - a.x)) < 0.001f);
        assert(std::fabs(square->getHeight() - std::fabs(b.y - a.y)) < 0.001f);
        // Flipped colliders are cached, not rebuilt per tile.
        assert(tiles->getCollision(0, flags) == tile.getState()->getCollidable());

        Tile slope;
        slope.setTileSet(tiles);
        slope.setTileIndex(30);
        const PolygonCollider* source = (const PolygonCollider*)slope.getState()->getCollidable();
        const PolygonCollider* flipped = (const PolygonCollider*)tiles->getCollision(30, flags);
        assert(source && flipped && flipped->getType() == COL_OBJ_POLYGON && flipped->isValid());
        std::vector<vector2> expected;
        for (const vector2& vertex : source->getLocalVertices()) {
            expected.push_back(TileSet::flipTilePoint(source->getPosition() + vertex, kSize, flags));
        }
        const std::vector<vector2>& actual = flipped->getLocalVertices();
        assert(actual.size() == expected.size());
        for (const vector2& point : expected) {
            bool found = false;
            for (const vector2& vertex : actual) found = found || near(flipped->getPosition() + vertex, point);
            assert(found);
        }
        // Winding is preserved so edge normals keep facing out of the surface.
        std::vector<vector2> sourceWorld;
        for (const vector2& vertex : source->getLocalVertices()) sourceWorld.push_back(vertex);
        assert((signedArea(actual) > 0.0f) == (signedArea(sourceWorld) > 0.0f));
    }

    // Map loading keeps the flag bits instead of discarding them.
    const char* mapPath = "tmp/tile_flip_test.tmj";
    std::ofstream map(mapPath);
    map << R"({"width":3,"height":1,"tilewidth":16,"tileheight":16,"orientation":"orthogonal",
        "tilesets":[{"firstgid":1,"source":"../bin/fantasySideScroller/Assets/fantasyTiles.tsj"}],
        "layers":[{"id":1,"name":"flips","type":"tilelayer","width":3,"height":1,"x":0,"y":0,
        "opacity":1,"visible":true,"data":[1,)" << (0x80000000u | 1u) << "," << (0xA0000000u | 31u) << "]}]}";
    map.close();
    std::vector<TileMap*> layers = TileMap::loadFromJSONFile(mapPath, nullptr, false);
    assert(layers.size() == 1);
    assert(layers[0]->getTile(0, 0)->getFlipFlags() == 0);
    assert(layers[0]->getTile(1, 0)->getFlipFlags() == H);
    assert(layers[0]->getTile(1, 0)->getTileIndex() == 0);
    assert(layers[0]->getTile(2, 0)->getFlipFlags() == (D | H));
    assert(layers[0]->getTile(2, 0)->getTileIndex() == 30);
    std::remove(mapPath);

    std::cout << "tile flip test passed\n";
    return 0;
}
