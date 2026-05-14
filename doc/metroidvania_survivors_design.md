# Metroidvania + Vampire Survivors Game Design

**Project**: `framework2d` → New Demo/Prototype  
**Date**: 2026-05-14  
**Author**: Stanley Taveras

---

## 0. One-Sentence Pitch

**"Explore a connected world, master combat through endless waves of enemies, and unlock new abilities to access previously inaccessible areas."**

A Metroidvania where combat power scales with survival runs, and ability unlocks expand the explorable world.

---

## 1. Core Philosophy

### Why This Design?
- **No microtransactions**: Progression is earned through skill and survival
- **Meaningful combat**: Each enemy encounter tests player skill
- **Exploration-driven**: Abilities unlock new paths, creating a sense of discovery
- **Progressive difficulty**: Wave intensity scales with player power

### Design Pillars
1. **Combat is king** - Your survival depends on mastering movement and attack patterns
2. **Exploration rewards** - Finding secrets and abilities unlocks new areas
3. **Permanent progression** - Abilities and upgrades persist across runs
4. **Risk vs. reward** - Dangerous areas contain the best loot and secrets

---

## 2. Core Loop

```
EXPLORE → ENGAGE → SURVIVE → PROGRESS → UNLOCK → REPEAT
```

### The Loop in Detail

1. **Explore the world map**
   - Navigate interconnected regions (rooms, corridors, dungeons)
   - Find secrets, collectibles, and ability upgrades
   - Some areas are locked behind ability gates or enemy waves

2. **Engage enemies**
   - Enemies spawn in waves, increasing in difficulty
   - Combat uses auto-attack + combo system (Vampire Survivors style)
   - Positioning and dodging are as important as attacking

3. **Survive the wave**
   - Clear all enemies to advance
   - Collect XP orbs to level up (3 choices per level)
   - Collect currency (gold/gems) to buy permanent upgrades

4. **Progress**
   - Choose ability upgrades at level-up
   - Buy weapon/passive upgrades with gold
   - Complete objectives for bonus rewards

5. **Unlock new areas**
   - Find ability gates/locks in the world map
   - Defeat area bosses to unlock progression
   - Return to previously inaccessible areas with new abilities

---

## 3. World Structure

### Map Type: Connected Tile Map
- **Format**: Single large tile map with interconnected rooms
- **Regions**: 5-7 distinct biomes/areas
- **Navigation**: Player can move freely except where blocked by:
  - Walls/terrain
  - Ability gates (require specific ability)
  - Enemy-controlled zones (high difficulty areas)

### Regions (Example)

| Region | Theme | Ability Gate | Boss |
|--------|-------|--------------|------|
| **1. Starting Cell** | Small safe room | None | Wave 10 |
| **2. Graveyard** | Foggy, dark paths | Torch/Light ability | Graveyard Guardian |
| **3. Crypt** | Underground tunnels | Darkness resistance | Crypt King |
| **4. Forest** | Overgrown, hidden paths | Climbing ability | Forest Spirit |
| **5. Ruins** | Ancient architecture, traps | Spell resistance | Ruin Overseer |
| **6. Castle** | Final area, boss arena | All abilities required | Final Boss |

### Ability Gates
- **Torch Gates**: Require light ability to pass
- **Dark Gates**: Require darkness resistance
- **Spiked Gates**: Require jump/flight ability
- **Locked Doors**: Require key items (found in secrets)

---

## 4. Combat System

### Player Actions

#### Auto-Attack (Vampire Survivors Style)
- **Basic Attack**: Continuous auto-attack based on weapon type
  - Sword: Melee, short range, fast
  - Spear: Melee, medium range, piercing
  - Bow: Ranged, medium range, slow
  - Staff: Ranged, slow, AoE projectiles
- **Attack Speed**: Increases with upgrades
- **Auto-targeting**: Attacks nearest enemy in range

#### Movement (Metroidvania Style)
- **Movement**: 4-directional (or 8 with diagonal)
- **Dashing**: Quick burst movement (cooldown-based)
- **Jumping**: Variable jump height, wall jumps possible
- **Sliding**: Low-profile to avoid tall attacks

#### Abilities (Level-Up Choices)

**Active Abilities** (player-triggered)
1. **Dash** - Quick burst in direction
2. **Shield** - Temporary invincibility
3. **Flash** - Stun nearby enemies
4. **Teleport** - Escape mechanic
5. **Bomb** - AoE damage

**Passive Abilities** (automatic bonuses)
1. **Area Damage** - Attacks hit multiple enemies
2. **Damage Boost** - Increased damage
3. **Speed** - Faster movement/attack
4. **Loot Magnet** - Auto-collect items
5. **XP Boost** - More XP per enemy

---

## 5. Enemy System

### Enemy Types (Tiered by Difficulty)

#### Tier 1 (Early Game)
- **Slimes**: Basic melee enemies
- **Rats**: Fast, low HP
- **Ghosts**: Float over walls, weak to light

#### Tier 2 (Mid Game)
- **Skeletons**: Fast, hit-and-run
- **Zombies**: Tanky, slow
- **Bats**: Flying, swarm behavior

#### Tier 3 (Late Game)
- **Wraiths**: Fast, invisible
- **Demon Knights**: Tanky, charge attacks
- **Cursed Zombies**: Summon minions

### Enemy Behaviors

| Behavior | Description |
|----------|-------------|
| **Chaser** | Follows player, stops at range |
| **Hunter** | Pursues player with pathfinding |
| **Swarm** | Moves in groups, surrounds player |
| **Ambusher** | Waits in hiding, attacks on proximity |
| **Flying** | Ignores ground obstacles |
| **Boss** | High HP, multiple attack patterns |

### Wave Progression

```
Wave 1-3:   Tutorial, basic enemies
Wave 4-10:  Introduce 2 enemy types, faster spawns
Wave 11-20: Add flying enemies, increased spawn rate
Wave 21-30: Introduce bosses, varied enemy mixes
Wave 31+:   All enemy types, maximum difficulty
```

---

## 6. Progression System

### XP & Leveling

- **XP Orbs**: Drop from defeated enemies
- **Level Up**: Every 3 orbs (configurable)
- **Choices**: 3 ability upgrades per level
- **Cap**: 50 levels (or higher with prestige)

### Currency

- **Gold**: Dropped from enemies, used for weapon upgrades
- **Gems**: Rare drops, used for special abilities
- **Souls**: Boss drops, used for major upgrades

### Upgrade Types

#### Weapon Upgrades
- **Damage**: +10%, +15%, +20%, +30% (max)
- **Speed**: Fire rate +10%, +15%, +20%
- **Range**: +20%, +30%, +40%
- **Ammo**: Unlimited, rapid refill, etc.

#### Passive Upgrades
- **Damage Bonus**: Global damage multiplier
- **Area Damage**: Increase AoE radius
- **Loot Magnet**: Pull items closer
- **XP Bonus**: More XP per enemy

#### Ability Upgrades (Level-Up Specific)
- **Dash**: Cooldown -20%, duration +20%
- **Shield**: Duration +20%, charge faster
- **Flash**: Cooldown -15%, duration +20%

---

## 7. Technical Architecture

### New Components Needed

#### 1. Enemy Spawner
```cpp
class EnemySpawner {
    // Wave management
    // Spawning logic (circular, linear, random)
    // Difficulty scaling
    // Boss management
}
```

#### 2. Combat System
```cpp
class CombatSystem {
    // Player attacks (auto-targeting)
    // Enemy AI (chase, flee, attack)
    // Damage calculation
    // Collision detection
}
```

#### 3. World Map
```cpp
class WorldMap {
    // Large tile map (seamless or level-based)
    // Ability gate checking
    // Area boundaries
    // Save/load state
}
```

#### 4. Upgrade System
```cpp
class UpgradeSystem {
    // Level-up choices
    // Weapon upgrades
    // Passive upgrades
    // Ability upgrades
    // Save/load state
}
```

#### 5. Save/Load
```cpp
class SaveSystem {
    // Save player state (position, level, upgrades)
    // Save world state (unlocked areas, progress)
    // Load on game start
}
```

### Integration with Existing Framework

- **Use**: `GameObject`, `Controller`, `CollisionSystem`, `TileMap`, `Camera`
- **Extend**: Add `Enemy` class, `UpgradeSystem`, `WorldMap`
- **Modify**: `GameState` to track wave progress, `Controller` for combat input

---

## 8. Demo Scope (MVP)

### Phase 1: Core Prototype (2-3 weeks)
- ✅ Basic player movement (4-directional)
- ✅ Auto-attack system (melee weapon)
- ✅ Basic enemy AI (chase + damage player)
- ✅ Wave spawner (fixed wave count)
- ✅ XP/leveling (3 ability choices)
- ✅ Single room demo

### Phase 2: World & Abilities (2-3 weeks)
- ✅ World map (connected rooms)
- ✅ Ability gates (1-2 types)
- ✅ 3-4 player abilities
- ✅ Enemy variety (5-6 types)
- ✅ Boss fight (1 boss)

### Phase 3: Polish & Content (2-3 weeks)
- ✅ More enemies (10+ types)
- ✅ More abilities (8+ choices)
- ✅ Weapon upgrades
- ✅ Visual polish (particles, effects)
- ✅ Save/load system
- ✅ Tutorial/intro level

---

## 9. Asset Requirements

### Needed
- **Player sprite** (idle, run, attack, hit, dead - 4 frames each)
- **Enemy sprites** (simple silhouettes to start)
- **Tileset** (rooms, corridors, walls, floors)
- **UI** (health bar, XP bar, level-up choices, upgrade menus)
- **Audio** (player footsteps, enemy hits, level-up sounds)

### Sources
- Free asset packs (itch.io, OpenGameArt)
- Procedural generation (for early prototyping)
- Simple geometric shapes (for initial testing)

---

## 10. Open Questions

1. **Platform first**: macOS (Metal) or Windows (DirectX/Vulkan)?
2. **Input scheme**: Keyboard only or add controller support later?
3. **Art direction**: Pixel art, vector, or minimalist geometric?
4. **Single-player only**: No multiplayer for MVP?
5. **Combat depth**: Simple auto-attack or add combo mechanics?

---

## 11. Next Steps

1. **Create MVP prototype** with:
   - Player movement
   - Basic enemy (chaser)
   - Wave spawner
   - Simple combat (auto-attack)

2. **Set up project structure**:
   - `src/MetroidvaniaSurvivors/`
   - Organize by systems (Combat, World, Enemies, Upgrades)

3. **Build core systems** one at a time, test frequently

---

*This design document is living and will be updated as development progresses.*
