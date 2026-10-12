// Checks tileset-driven uv scrolling: uv_scroll_* properties resolve to the strip a tile
// loops through, and touching scrolling tiles sample one continuous strip.
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
    Sprite::UVScrollSegment at[2];
    size_t count = 0;
};

// The source rows a tile draws, as the renderers request them.
Segments segmentsOf(Tile& tile)
{
    Segments segments;
    const Image* image = (const Image*)tile.getState()->getRenderable();
    segments.count = image->getVerticalUVScrollSegments(segments.at, 2);
    return segments;
}

void placeTile(Tile& tile, TileSet* tiles, int tileIndex, vector2 position)
{
    tile.setTileSet(tiles);
    tile.setTileIndex(tileIndex);
    tile.setPosition(position);
    tile.advanceVerticalUVScroll(0.0f);
}

void assertRegion(const TileSet* tiles, int tileIndex, int top, int height)
{
    const TileSet::TileInfo* info = tiles->findTileInfo(tileIndex);
    assert(info && info->_uvScroll.isActive());
    assert(info->_uvScroll.regionTop == top);
    assert(info->_uvScroll.regionHeight == height);
}

} // namespace

int main()
{
    System::GlobalDataPath("bin/fantasySideScroller");
    TestRenderer renderer;
    Engine2D::setRenderer(&renderer);

    // The water block (columns 3-5, rows 17-20) is one 64 px strip scrolling down.
    TileSet* tiles = TileSet::loadFromFile("bin/fantasySideScroller/Assets/fantasyTiles.tsj");
    assert(tiles);
    for (int tileIndex : {428, 429, 430, 453, 454, 455, 478, 479, 480, 503, 504, 505}) {
        assertRegion(tiles, tileIndex, 17 * 16, 64);
        assert(tiles->findTileInfo(tileIndex)->_uvScroll.speed == 75.0f);
        assert(tiles->findTileInfo(tileIndex)->_uvScroll.direction == TileUVScrollDirection::Down);
    }
    assert(!tiles->findTileInfo(431) || !tiles->findTileInfo(431)->_uvScroll.isActive());

    // A strip is the run of same-scrolling tiles in one sheet column: a different speed or
    // direction starts a new strip, and a lone tile loops over itself.
    const char* tileSetPath = "tmp/tile_uv_scroll_test.tsj";
    std::ofstream tileSet(tileSetPath);
    tileSet << R"({"image":"../bin/fantasySideScroller/Assets/Tiles.png","tilewidth":16,"tileheight":16,
        "tilecount":625,"columns":25,"tiles":[)"
        R"({"id":25,"properties":[{"name":"uv_scroll_speed","type":"float","value":10}]},)"
        R"({"id":50,"properties":[{"name":"uv_scroll_speed","type":"float","value":10}]},)"
        R"({"id":75,"properties":[{"name":"uv_scroll_speed","type":"float","value":10}]},)"
        R"({"id":100,"properties":[{"name":"uv_scroll_speed","type":"float","value":20}]},)"
        R"({"id":51,"properties":[{"name":"uv_scroll_speed","type":"float","value":10},)"
        R"({"name":"uv_scroll_direction","type":"string","value":"up"}]},)"
        R"({"id":76,"properties":[{"name":"uv_scroll_speed","type":"float","value":10}]}]})";
    tileSet.close();
    TileSet* strips = TileSet::loadFromFile(tileSetPath);
    std::remove(tileSetPath);
    assert(strips);
    assertRegion(strips, 25, 16, 48);
    assertRegion(strips, 50, 16, 48);
    assertRegion(strips, 75, 16, 48);
    assertRegion(strips, 100, 64, 16);
    assertRegion(strips, 51, 32, 16);
    assert(strips->findTileInfo(51)->_uvScroll.direction == TileUVScrollDirection::Up);
    assertRegion(strips, 76, 48, 16);

    // Stacked copies of one tile continue the strip instead of repeating their own row, and
    // the tile's row in the sheet no longer matters - only where it sits in the world.
    Tile upper, lower, otherRow, beside;
    placeTile(upper, tiles, 428, vector2(0, 0));
    placeTile(lower, tiles, 428, vector2(0, 16));
    placeTile(otherRow, tiles, 503, vector2(32, 16));
    placeTile(beside, tiles, 429, vector2(16, 0));
    assert(upper.hasUVScroll() && lower.hasUVScroll());
    assert(segmentsOf(upper).count == 1 && segmentsOf(upper).at[0].sourceTop == 272.0f);
    assert(segmentsOf(lower).count == 1 && segmentsOf(lower).at[0].sourceTop == 288.0f);
    assert(segmentsOf(otherRow).at[0].sourceTop == 288.0f);
    assert(segmentsOf(beside).at[0].sourceTop == 272.0f);

    // 0.1 s at 75 px/s moves the art 7.5 px down (7 whole source rows). The upper tile
    // wraps across the strip's bottom edge, and its last row meets the lower tile's first.
    for (Tile* tile : {&upper, &lower, &otherRow, &beside}) {
        tile->advanceVerticalUVScroll(0.1f);
    }
    const Segments top = segmentsOf(upper);
    const Segments bottom = segmentsOf(lower);
    assert(top.count == 2);
    assert(top.at[0].sourceTop == 329.0f && top.at[0].sourceBottom == 336.0f);
    assert(top.at[1].sourceTop == 272.0f && top.at[1].sourceBottom == 281.0f);
    assert(bottom.count == 1 && bottom.at[0].sourceTop == top.at[1].sourceBottom);
    assert(segmentsOf(otherRow).at[0].sourceTop == bottom.at[0].sourceTop);
    assert(segmentsOf(beside).at[0].sourceTop == top.at[0].sourceTop);

    // A cleared tile (e.g. a collected pickup) stops scrolling.
    lower.setTileIndex(-1);
    assert(!lower.hasUVScroll());

    std::cout << "tile uv scroll test passed\n";
    return 0;
}
