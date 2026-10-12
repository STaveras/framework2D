// Checks uv-looping tiles: the tileset's uv_loop_x/uv_loop_y names resolve to the strips
// tiles loop through, map layers (and their groups) say how the art moves, and runs of
// moving tiles sample one continuous strip.
#include "../src/Engine2D.h"
#include "../src/System.h"
#include "../src/TileMap.h"
#include "stb/stb_image.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

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

struct Segments {
    Sprite::UVSegment at[4];
    size_t count = 0;
};

// The source pieces a tile draws, as the renderers request them.
Segments segmentsOf(Tile* tile)
{
    Segments segments;
    const Image* image = (const Image*)tile->getState()->getRenderable();
    segments.count = image->getUVSegments(segments.at, 4);
    return segments;
}

void assertLoop(const TileSet* tiles, int tileIndex, int left, int width, int top, int height)
{
    const TileSet::TileInfo* info = tiles->findTileInfo(tileIndex);
    assert(info);
    assert(info->_uvLoop.width == width && (width == 0 || info->_uvLoop.left == left));
    assert(info->_uvLoop.height == height && (height == 0 || info->_uvLoop.top == top));
}

} // namespace

int main()
{
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);

    // The waterfall block (columns 3-5, rows 17-20) loops vertically through 64 px; each
    // pond row (columns 6-9, rows 18-20) loops horizontally through 64 px.
    TileSet* tiles = TileSet::loadFromFile("bin/fantasySideScroller/Assets/fantasyTiles.tsj");
    assert(tiles);
    for (int tileIndex : {428, 429, 430, 453, 454, 455, 478, 479, 480, 503, 504, 505}) {
        assertLoop(tiles, tileIndex, 0, 0, 17 * 16, 64);
    }
    for (int tileIndex : {456, 457, 458, 459, 481, 482, 483, 484, 506, 507, 508, 509}) {
        assertLoop(tiles, tileIndex, 6 * 16, 64, 0, 0);
    }
    assert(!tiles->findTileInfo(431) || tiles->findTileInfo(431)->_uvLoop.width + tiles->findTileInfo(431)->_uvLoop.height == 0);

    // A strip is the run of tiles in one sheet column (row) carrying the same uv_loop_y
    // (uv_loop_x) name: another name starts a new strip, and a lone tile loops over itself.
    const char* tileSetPath = "tmp/tile_uv_scroll_test.tsj";
    std::ofstream tileSet(tileSetPath);
    tileSet << R"({"image":"../bin/fantasySideScroller/Assets/Tiles.png","tilewidth":16,"tileheight":16,
        "tilecount":625,"columns":25,"tiles":[)"
        R"({"id":25,"properties":[{"name":"uv_loop_y","type":"string","value":"a"}]},)"
        R"({"id":50,"properties":[{"name":"uv_loop_y","type":"string","value":"a"}]},)"
        R"({"id":75,"properties":[{"name":"uv_loop_y","type":"string","value":"a"}]},)"
        R"({"id":100,"properties":[{"name":"uv_loop_y","type":"string","value":"b"}]},)"
        R"({"id":51,"properties":[{"name":"uv_loop_x","type":"string","value":"c"}]},)"
        R"({"id":52,"properties":[{"name":"uv_loop_x","type":"string","value":"c"}]},)"
        R"({"id":53,"properties":[{"name":"uv_loop_x","type":"string","value":"d"}]}]})";
    tileSet.close();
    TileSet* strips = TileSet::loadFromFile(tileSetPath);
    std::remove(tileSetPath);
    assert(strips);
    assertLoop(strips, 25, 0, 0, 16, 48);
    assertLoop(strips, 75, 0, 0, 16, 48);
    assertLoop(strips, 100, 0, 0, 64, 16);
    assertLoop(strips, 51, 16, 32, 0, 0);
    assertLoop(strips, 52, 16, 32, 0, 0);
    assertLoop(strips, 53, 48, 16, 0, 0);

    // Motion comes from the map: a group's properties reach its layers, a layer's own
    // override them, and a layer without any leaves its tiles as painted. gid = index + 1.
    const char* mapPath = "tmp/tile_uv_scroll_test.tmj";
    std::ofstream map(mapPath);
    map << R"({"width":3,"height":3,"tilewidth":16,"tileheight":16,"orientation":"orthogonal",
        "tilesets":[{"firstgid":1,"source":"../bin/fantasySideScroller/Assets/fantasyTiles.tsj"}],
        "layers":[
        {"id":1,"name":"falls","type":"group","visible":true,"opacity":1,"x":0,"y":0,
         "properties":[{"name":"uv_scroll_y","type":"float","value":75}],
         "layers":[
          {"id":2,"name":"column","type":"tilelayer","width":1,"height":3,"x":0,"y":0,"opacity":1,"visible":true,
           "data":[429,429,429]},
          {"id":3,"name":"upstream","type":"tilelayer","width":1,"height":1,"x":0,"y":0,"opacity":1,"visible":true,
           "properties":[{"name":"uv_scroll_y","type":"float","value":-20}],"data":[429]}]},
        {"id":4,"name":"still","type":"tilelayer","width":1,"height":3,"x":0,"y":0,"opacity":1,"visible":true,
         "data":[429,429,429]},
        {"id":5,"name":"pond","type":"tilelayer","width":3,"height":1,"x":0,"y":0,"opacity":1,"visible":true,
         "properties":[{"name":"uv_sway_x","type":"float","value":4},{"name":"uv_sway_period","type":"float","value":2}],
         "data":[460,457,458]}]})";
    map.close();
    std::vector<TileMap*> layers = TileMap::loadFromJSONFile(mapPath);
    std::remove(mapPath);
    assert(layers.size() == 4);
    TileMap* column = layers[0];
    TileMap* upstream = layers[1];
    TileMap* still = layers[2];
    TileMap* pond = layers[3];
    assert(column->getLayerConfig().uvMotion.scroll.y == 75.0f);
    assert(upstream->getLayerConfig().uvMotion.scroll.y == -20.0f);
    assert(!still->getLayerConfig().uvMotion.movesX() && !still->getLayerConfig().uvMotion.movesY());
    assert(pond->getLayerConfig().uvMotion.sway.x == 4.0f && pond->getLayerConfig().uvMotion.swayPeriod == 2.0f);

    // Painted as three copies of tile 428, the moving column continues the strip down from
    // its first tile, while the still layer draws each copy as painted.
    for (unsigned int y = 0; y < 3; ++y) {
        assert(column->getTile(0, y)->hasUVMotion());
        assert(!still->getTile(0, y)->hasUVMotion());
        const Segments painted = segmentsOf(still->getTile(0, y));
        assert(painted.count == 1 && painted.at[0].sourceTop == 272.0f && painted.at[0].sourceLeft == 48.0f);
        const Segments linked = segmentsOf(column->getTile(0, y));
        assert(linked.count == 1 && linked.at[0].sourceTop == 272.0f + 16.0f * y && linked.at[0].sourceLeft == 48.0f);
    }

    // 0.1 s at 75 px/s moves the art 7.5 px down (7 whole source rows). The top tile wraps
    // across the strip's bottom edge, and its last row meets the next tile's first.
    for (unsigned int y = 0; y < 3; ++y) {
        column->getTile(0, y)->updateUVLoop(0.1);
    }
    const Segments top = segmentsOf(column->getTile(0, 0));
    const Segments middle = segmentsOf(column->getTile(0, 1));
    assert(top.count == 2);
    assert(top.at[0].sourceTop == 329.0f && top.at[0].sourceBottom == 336.0f);
    assert(top.at[1].sourceTop == 272.0f && top.at[1].sourceBottom == 281.0f);
    assert(top.at[0].destinationBottom == top.at[1].destinationTop);
    assert(middle.count == 1 && middle.at[0].sourceTop == top.at[1].sourceBottom);

    // The pond row is painted 459, 456, 457: the run keeps its first tile's painted spot
    // and continues the strip from there. A quarter of the 2 s sway (t = 0.5) moves the art
    // 4 px right, three quarters 4 px left; rows are never looped.
    for (unsigned int x = 0; x < 3; ++x) {
        assert(pond->getTile(x, 0)->hasUVMotion());
        const Segments rest = segmentsOf(pond->getTile(x, 0));
        assert(rest.count == 1 && rest.at[0].sourceLeft == 96.0f + 16.0f * ((x + 3) % 4));
        pond->getTile(x, 0)->updateUVLoop(0.5);
    }
    const Segments left = segmentsOf(pond->getTile(0, 0));
    const Segments centre = segmentsOf(pond->getTile(1, 0));
    const Segments right = segmentsOf(pond->getTile(2, 0));
    assert(left.count == 1 && left.at[0].sourceLeft == 140.0f && left.at[0].sourceRight == 156.0f);
    assert(centre.count == 2);
    assert(centre.at[0].sourceLeft == 156.0f && centre.at[0].sourceRight == 160.0f);
    assert(centre.at[1].sourceLeft == 96.0f && centre.at[1].sourceRight == 108.0f);
    assert(centre.at[0].sourceTop == 288.0f && centre.at[0].sourceBottom == 304.0f);
    assert(right.count == 1 && right.at[0].sourceLeft == centre.at[1].sourceRight);
    pond->getTile(1, 0)->updateUVLoop(1.5);
    assert(segmentsOf(pond->getTile(1, 0)).at[0].sourceLeft == 100.0f);

    // A cleared tile (e.g. a collected pickup) stops moving.
    column->getTile(0, 2)->setTileIndex(-1);
    assert(!column->getTile(0, 2)->hasUVMotion());

    std::cout << "tile uv scroll test passed\n";
    return 0;
}
