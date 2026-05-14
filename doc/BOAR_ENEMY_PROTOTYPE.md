# Boar Enemy Prototype

**Date**: 2026-05-14  
**Branch**: `dev/metroidvania_survivors`  
**Files Created**: `FantasySideScroller/Enemy.h`, `FantasySideScroller/Enemy.cpp`, `FantasySideScroller/EnemyTest.cpp`

---

## Overview

Created a base enemy class (`Enemy`) that derives from the existing `Character` class, reusing its movement, physics, and animation systems. This provides a solid foundation for the Metroidvania + Vampire Survivors game.

---

## Design Decisions

### 1. Inheritance from Character
**Why**: The existing `Character` class already handles:
- Movement and physics
- Collision detection
- Animation states
- Input handling

**Benefit**: No need to reimplement these systems - just extend for enemy-specific behavior.

### 2. Enemy-Specific Additions
Added to the base class:
- **Health system** (separate from stamina)
- **AI state machine** (Patrol, Chase, Attack, Flee, Wait)
- **Combat mechanics** (damage, attack cooldowns)
- **Death/respawn handling**

### 3. Enemy Types
Implemented enum for different enemy types:
- **BOAR**: Melee, fast, low HP (base template)
- **BEE**: Flying, swarm behavior
- **SNAIL**: Slow, tanky
- **SKELETON**: Fast, hit-and-run
- **ZOMBIE**: Tanky, slow
- **BAT**: Flying, swarm

Each type has predefined stats (health, damage, speed, detection range, etc.).

---

## Enemy Behavior

### Patrol Mode (Default)
- Moves in a circle around a patrol position
- Slow movement speed (50% of max)
- Good for populating levels without constant aggression

### Chase Mode
- Activated when player enters detection range
- Moves directly toward player
- Stops if player is out of detection range

### Attack Mode
- Activated when player is in attack range (aggro range)
- Could add attack animations/damage here
- Currently just stays in position

### AI State Machine
```
PATROL → (detect player) → CHASE → (in range) → ATTACK
                              ↓
                          (out of range) → WAIT
```

---

## Damage System

```cpp
void takeDamage(float amount, GameObject* attacker)
```

**Features**:
- **Hit cooldown** (prevents rapid damage)
- **Invulnerability frames** (0.5s after taking damage)
- **Death check** (resets when health ≤ 0)
- **Visual feedback** (flash white when hit)

---

## Spawning and Management

### Factory Method
```cpp
Enemy* create(Type type, const vector2& position)
```

**Usage**:
```cpp
Enemy* boar = Enemy::create(Enemy::Type::BOAR, vector2(200.0f, 300.0f));
```

### Object Manager Integration
Enemies are added to the existing `ObjectManager` like any other GameObject:
```cpp
ObjectManager& objManager = Engine2D::getObjectManager();
objManager.addGameObject(boar);
```

---

## Reusing Existing Assets

### Character Sprites
The Enemy class can reuse the existing Character assets:
- **Idle**: Enemy idle animation
- **Run**: Enemy movement animation
- **Attack**: Enemy attack animation (if Attack-01 is suitable)
- **Dead**: Enemy death animation

### Animation States
The existing `_initStates()` and `_initTransitions()` methods can be overridden for enemy-specific animations.

---

## Next Steps

### Immediate
1. **Test the prototype** - Run EnemyTest.cpp to verify basic functionality
2. **Add enemy sprites** - Replace Character sprites with enemy-specific assets
3. **Implement enemy AI** - Add more sophisticated behavior (pathfinding, grouping, etc.)

### Short Term
4. **Add enemy variety** - Implement different enemy types with unique behaviors
5. **Wave spawner** - Create system to spawn enemies in waves
6. **Combat feedback** - Add hit animations, damage numbers, etc.

### Medium Term
7. **Boss enemies** - Large, intimidating enemies with multiple attack patterns
8. **AI improvements** - Better pathfinding, grouping, coordination
9. **Enemy upgrades** - Make enemies stronger as player progresses

---

## Files Modified

### New Files
- `src/FantasySideScroller/Enemy.h` - Enemy class header
- `src/FantasySideScroller/Enemy.cpp` - Enemy class implementation
- `src/FantasySideScroller/EnemyTest.cpp` - Test/demo code

### Existing Files (Unchanged)
- All Character, TileMap, LevelManager, etc. files remain intact
- FantasySideScroller demo preserved as-is

---

## Build Instructions

The new files are automatically picked up by CMake (uses GLOB). To build:

```bash
cd /home/stan/.openclaw/workspace/stan/framework2d
mkdir -p build
cd build
cmake ..
cmake --build . --config Debug
```

The executable will be at `bin/framework_d.exe` (Windows) or `bin/framework_d` (Linux/Mac).

---

## Notes

- **Hit cooldown**: 1.0 second between damage instances (prevents instant death)
- **Detection range**: 200 units (configurable per enemy type)
- **Aggro range**: 150 units (enemy engages when player in this range)
- **Respawn time**: 3.0 seconds after death
- **Invulnerability**: 0.5 seconds after taking damage

---

*This prototype provides a foundation for enemy AI that can be expanded with more sophisticated behaviors and enemy types.*
