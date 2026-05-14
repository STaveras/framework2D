# Asset Inventory & Reuse Plan

**Date**: 2026-05-14  
**For**: Metroidvania + Vampire Survivors Prototype

---

## Existing Assets Overview

The current `framework2d` project has a solid asset foundation from the Fantasy Side Scroller demo. Here's what we have and how to repurpose it:

---

## 1. Character Assets (Player)

### Current Location
`bin/fantasySideScroller/Character/`

### Available Sprites
- **Idle** - Standing animation (4 frames)
- **Run** - Movement animation (4 frames)
- **Jump** - Jumping animation (3 frames)
- **Jump-Start** - Jump initiation
- **Jump-End** - Jump recovery
- **Jump-All** - Extended jump sequence
- **Attack-01** - Melee attack animation
- **Dead** - Death animation

### Reuse Plan
- ✅ **Directly usable** as player sprite
- **Modify**: Add dash, slide, and wall-jump animations
- **Current animations fit**: Metroidvania movement perfectly
- **Attack-01**: Can be base for auto-attack system

---

## 2. Enemy/Mob Assets

### Current Location
`bin/fantasySideScroller/Mob/`

### Available Sprites
- **Boar** - Idle, Walk, Run, Hit-Vanish (4 animations)
- **Small Bee** - Flying enemy template
- **Snail** - Slow, tanky enemy template

### Reuse Plan
- ✅ **Perfect for Vampire Survivors enemy variety**
- **Boar**: Can be melee enemy (Tier 1)
- **Bee**: Flying enemy (Tier 2-3)
- **Snail**: Slow, tanky enemy (Tier 1-2)
- **Add**: Hit/stun frames for combat feedback

---

## 3. Tilemap Assets

### Current Location
`bin/fantasySideScroller/`

### Available Maps
- **testMap.tmj** - 8x8 tile test map
- **mockup_tiles2.tmj** - Larger mockup (31KB)
- **testMap_separate_layers.tmj** - Multi-layer map

### Tilesets
- **Tiles.aseprite/png** - General tiles
- **DebugTiles.aseprite/png** - Debug visualization
- **Buildings.aseprite/png** - Structure tiles
- **Props-Rocks.aseprite/png** - Environmental props
- **Tree-Assets.aseprite/png** - Vegetation
- **Hive.aseprite/png** - Special structure
- **Interior-01.png** - Indoor tiles

### Reuse Plan
- ✅ **mockup_tiles2.tmj** is ideal for Metroidvania world
- **Tile variety supports**: Multiple biome regions
- **Separate layers**: Perfect for ability gates (transparent = blocked)
- **Props-Rocks/Tree-Assets**: Environmental hazards/decor

---

## 4. Background Assets

### Current Location
`bin/fantasySideScroller/Background/`

### Available
- **Background.aseprite/png** - Parallax background layers

### Reuse Plan
- ✅ **Use for each region** with different color tints
- **Graveyard**: Dark, foggy tint
- **Forest**: Green, overgrown tint
- **Ruins**: Gray, stone tint
- **Castle**: Purple/mystical tint

---

## 5. HUD/UI Assets

### Current Location
`bin/fantasySideScroller/HUD/`

### Available
- **Base-01.aseprite/png** - HUD base layer
- **pixel.bmp** - 1x1 pixel (used for filling bars)

### Current HUD Display
- Stamina bar (green fill)
- XP/Level text display ("exp: 0; lvl. 1")
- Bitmap font (monogram-bitmap.json)

### Reuse Plan
- ✅ **Perfect for Vampire Survivors HUD**
- **Modify stamina bar** → **Health bar**
- **Add**:
  - XP bar (visual, not just text)
  - Wave counter
  - Level-up menu overlay
  - Upgrade selection screen
  - Health pickup display
  - Currency display (gold/gems)

---

## 6. Fonts

### Current Location
`bin/fantasySideScroller/Font/`

### Available
- **monogram-bitmap.json** - Pixel bitmap font
- **pico-8_font_022.png** + **pico-8_palette_020.png** - Pico-8 style font

### Reuse Plan
- ✅ **Bitmap font works** for retro aesthetic
- ✅ **Pico-8 font** is perfect for Metroidvania vibe
- **Use Pico-8** for UI, **monogram** for in-game text if needed

---

## 7. Missing Assets (To Create/Acquire)

### Priority 1: Player Combat
- **Dash animation** (2-3 frames, quick blur effect)
- **Slide animation** (low-profile movement)
- **Attack frames** (if Attack-01 not sufficient)
- **Hit/stunned frames** (for enemy combat feedback)

### Priority 2: Enemy Variety
- **Skeletal enemies** (for Graveyard/Crypt regions)
- **Flying bats** (for Forest region)
- **Zombie-like enemies** (for Crypt region)
- **Boss sprite** (large, intimidating)

### Priority 3: World Map Elements
- **Ability gate sprites** (torch, darkness, spike symbols)
- **Locked door sprites**
- **Key item sprites**
- **Checkpoint/Save markers**

### Priority 4: UI/UX
- **Level-up choice menu** (3 option buttons)
- **Upgrade selection screen**
- **Wave complete notification**
- **Death screen** (with restart option)
- **World map UI** (to show connected regions)

---

## 8. Technical Asset Structure

### Recommended Organization for New Project

```
bin/MetroidvaniaSurvivors/
├── Character/
│   ├── Idle/
│   ├── Run/
│   ├── Attack/
│   ├── Dash/
│   ├── Slide/
│   ├── Jump/
│   ├── WallJump/
│   └── Hit/
├── Enemies/
│   ├── Boar/
│   ├── Bee/
│   ├── Snail/
│   ├── Skeleton/
│   ├── Bat/
│   ├── Zombie/
│   └── Boss/
├── World/
│   ├── Tiles/
│   ├── Props/
│   ├── AbilityGates/
│   ├── Doors/
│   └── Checkpoints/
├── Backgrounds/
│   ├── StartingCell/
│   ├── Graveyard/
│   ├── Crypt/
│   ├── Forest/
│   ├── Ruins/
│   └── Castle/
├── UI/
│   ├── HUD/
│   ├── Menus/
│   └── Notifications/
├── Audio/
│   ├── SFX/
│   └── Music/
└── Maps/
    ├── world_map.tmj
    ├── region_01.tmj
    ├── region_02.tmj
    └── ...
```

---

## 9. Asset Reuse Strategy

### Phase 1: MVP (Core Prototype)
- **Character**: Reuse existing sprite, add simple dash/slide
- **Enemies**: Boar (melee), Bee (flying), Snail (tank)
- **World**: Use mockup_tiles2.tmj, modify for single room
- **HUD**: Reuse existing, modify for health/XP/wave
- **Background**: Single background with tint variation

### Phase 2: World & Abilities
- **Add**: Ability gate sprites (simple geometric shapes to start)
- **Add**: More enemy types (skeletons, bats, zombies)
- **Add**: Region-specific backgrounds (tinted versions)
- **Add**: Boss sprite (scaled-up enemy)

### Phase 3: Polish & Content
- **Add**: Full enemy variety (10+ types)
- **Add**: Complete UI (menus, notifications)
- **Add**: Audio (SFX, music)
- **Add**: Particle effects (combat, pickups)

---

## 10. Recommended Next Steps

### Immediate (This Session)
1. **Copy assets** to new project structure
2. **Modify character** animations (add dash/slide frames)
3. **Adapt HUD** for health/XP/wave display
4. **Create simple enemy** AI (chase player)

### Short Term (Next 1-2 Sessions)
1. **Add enemy variety** (skeletons, bats)
2. **Create ability gate** system (visual + logic)
3. **Implement wave spawner**
4. **Build XP/leveling** system

### Medium Term (Next 1-2 Weeks)
1. **Expand world** to multiple regions
2. **Add boss fights**
3. **Polish combat** (hit feedback, animations)
4. **Add upgrade system**

---

## 11. Asset Sources for Missing Items

### Free Sources
- **OpenGameArt.org** - 2D game assets
- **itch.io** - Free asset packs (filter by "free")
- **Kenney.nl** - Free game assets (CC0)
- **Craftpix** - Free pixel art packs

### Create Your Own
- Use **Aseprite** (if available) or **Piskel** (free)
- Start with **simple geometric shapes** for prototyping
- **Silhouettes** work well for enemy variety
- **Consistent color palette** (4-6 colors max)

---

*This document will be updated as assets are added or replaced during development.*
