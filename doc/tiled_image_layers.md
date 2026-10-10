# Tiled image layers (plan)

Status: planned, not started. Earlier work lives in the `archive/dev/level` tag
(Feb 2026); this plan starts fresh on the current loader instead of reviving it.

## Why

Tiled image layers place one whole picture (a sky, a backdrop band, a reference
mockup) on its own layer, with the same offset, parallax, tint and opacity
controls as tile layers. They would let a map declare its own sky instead of the
hard-coded `Background/Background.png`, and make parallax backdrops whole images
rather than tile mosaics.

Today the loader ignores them. `TileMap::loadMapDataFromJSONFile` records an
`imagelayer` in `result.layers` (so it gets an empty render list) but never reads
its image. `testMap.tmj` and `testMap_separate_layers.tmj` each carry an empty
`Image Layer 1` that must keep loading quietly.

## Scope

In:

- `image`, resolved against the map's directory like tileset sources
  (`FileSystem::Path::ResolveFromBaseOrParent`, as at `TileMap.h` tileset loading).
- `offsetx`/`offsety`, `parallaxx`/`parallaxy`, `opacity`, `tintcolor` and
  `visible`, all combined with parent group layers through `LayerGroupState`
  exactly as tile layers already do.
- `repeatx`/`repeaty`.
- `transparentcolor`, passed as the colour key to `Sprite(filePath, clearColor)`.
- `class`, for the reference-layer convention below.

Out:

- Collision. Image layers are art only; a `collision_mode` property on one is
  ignored with a debug message. Collision stays layer metadata on tile layers.
- Tile objects (objects with a `gid`) drawn as sprites, and image-collection
  tilesets. `archive/dev/level` did the former with a `Prop` GameObject driven by
  `prop_json`; gameplay props are now tile-driven (`LevelProps`), so that
  approach is not coming back.
- Terrain or gameplay art. Levels must stay clean wave-function-collapse input,
  so playable terrain stays in tiles; image layers are for backdrop, sky and
  reference images.

## Design

### Parsing (engine, `src/TileMap.h`)

Add a `TileImageLayerDescriptor` and `TileMapLoadResult::imageLayers`, filled by
an `imagelayer` branch in `processLayer` beside the `objectgroup` branch:

```
struct TileImageLayerDescriptor
{
	int id = -1;
	std::string name;
	std::string className;
	std::string imagePath;           // resolved; empty when the layer has none
	bool visible = true;
	int traversalIndex = -1;
	float offsetX = 0.0f, offsetY = 0.0f;     // includes group offsets
	float parallaxX = 1.0f, parallaxY = 1.0f; // includes group parallax
	float opacity = 1.0f;                     // includes group opacity
	uint32_t tintColor = 0xFFFFFFFFu;         // includes group tint
	uint32_t transparentColor = 0;            // 0 = none
	bool repeatX = false, repeatY = false;
};
```

Set `layerDescriptor.typedIndex` to its index, as the other layer types do. The
legacy `x`/`y` fields are always 0 for image layers in current Tiled; read them
only as `objectgroup` does, added to the offset.

### Placement (game, `LevelManager`)

Image layers need no GameObject: they never collide, update or get queried. Each
becomes plain `Image` renderables pushed straight into that layer's existing
render list (`_mapLayerRenderLists[traversalIndex]`), the way `_background`
sits in the base list. `LevelManager` owns them in an `_imageLayerSprites`
vector.

- Do this in `loadMapDataIntoObjectManager` after the tile-layer loop and before
  `refreshLevelBounds`, so draw order follows Tiled's layer order with no extra
  work: every layer already has a render list in traversal order.
- Give the list the layer's parallax and the map's parallax origin, exactly as
  the tile-layer loop does (`layerRenderList->parallaxX` and the next three
  lines).
- Position the image's top-left at `mapOffset + offset`. Apply tint and opacity
  as tiles do (`Color tint(tintColor); tint.a *= opacity`).
- Skip layers that are hidden, have no image, or fail to load, with a debug
  message naming the layer. Skipped layers cost nothing.
- Image layers do not extend the level bounds. A wide sky must not let the
  camera roam past the playable map.

Parallax 0 locks a layer to the screen, so a map can declare its sky as an image
layer with `parallaxx = parallaxy = 0`. That replaces the camera-pinned
`_background` path (`initialize`'s `backgroundFileName`, moved each frame in
`LevelManager::update`) for maps that opt in. The hard-coded background stays
the fallback until every map has its own.

### Repeating

Every backend samples with clamp-to-edge (`TextureGL.cpp`, the Vulkan sampler in
`RendererVK.cpp`, the MSL sampler in `RendererMTL.mm`), and repeating by UV wrap
would also break once textures share an atlas. So repeat with copies instead:

- At load, cover the range the layer can ever show. The camera is clamped to the
  level bounds, so the layer's camera range is
  `origin + (cameraRange - origin) * parallax`, widened by half the view
  (`screen size / zoom`) on each side. Place `ceil(range / imageSize) + 1`
  copies from the offset, stepping by the image size on each repeating axis.
- Copies are ordinary static sprites, so the existing chunk culling skips the
  off-screen ones and there is no per-frame cost.
- Rebuild the copies when the zoom changes. Only the debug zoom keys change it
  today (`PlayState.cpp`, under `DEBUGGING`).
- If the camera ever becomes unbounded, switch to a fixed ring of copies
  repositioned each frame from the list's parallax camera position.

### Lifetime

`shutdown` already destroys the layer render lists before unregistering tiles;
delete `_imageLayerSprites` after that and clear the vector. The F5 stage reload
goes through `shutdown`/`initialize`, so it needs nothing extra; check that it
leaks no textures or sprites and doesn't duplicate them.

The renderer caches textures by file name only (`IRenderer::_textureExists`), so
two layers using the same file with different `transparentcolor` values would
share the first one's key. Note it in a debug message rather than extending the
cache.

### Reference images

The `village.png` mockup on `archive/dev/level` was a target to design around in
Tiled. Image layers with class `reference` are skipped at runtime even when left
visible, so a mockup can stay visible while editing without shipping.

## Verification

- A small test map (`image_layer_test.tmj`, started with `AUTO_START_MAP`) with
  an offset image inside an offset group, a parallax 0 sky, a `repeatx` band at
  0.4, a tinted layer at half opacity, a hidden layer, a `reference` layer, and
  an empty one. Capture it under OpenGL, Vulkan and Metal, and diff GL against
  VK as for other rendering work.
- Scroll the full width with an input replay: no seams between repeat copies and
  no gaps at the level edges, also after debug zoom out.
- `old_mine_trail` has no image layers, so the benchmark must not move.
- Both test maps with the empty `Image Layer 1` still load cleanly.
- On iOS, images must sit under `bin/fantasySideScroller` (the bundled data
  directory) and load from the read-only bundle.

## Open questions

- Should the `reference` class convention be a class, or a boolean property?
- Should Old Mine Trail's tile-built backdrop planes (sky 0.2, misty 0.4,
  teal 0.6, pines 0.8) move to image layers once this lands? Default: no; the
  hand-edited map stays as is unless asked.

## Salvage from `archive/dev/level`

Use these for reference only; master's loader is group-aware and differs.

- Parsing sketch: `git show archive/dev/level:src/TileMap.h` (search
  `imagelayer`).
- Assets not on master: `rockProps.tsj`, `treeAssetsTiles.tsj`,
  `buildingsTiles.tsj` and `village.png`. Restore one with
  `git checkout archive/dev/level -- <path>`.
