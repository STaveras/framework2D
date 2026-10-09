#pragma once

#include "../GameObject.h"
#include "../Square.h"
#include "../BlinkFlash.h"

#include <vector>

class Character;
class ObjectManager;

// Position is the center of a 48x32 sprite; feet are 14 pixels below it.
class Boar : public GameObject
{
    ObjectManager& _world;
    Character& _target;
    // The sprite's head projects to the left in its authored orientation.
    // Keep the body slightly offset so the generic mirror path has a
    // directional shape to reflect when the boar turns around.
    Square _body{vector2(-14.0f, -8.0f), 32.0f, 22.0f};
    vector2 _spawn;
    int _direction = -1;
    float _pause = 2.0f;
    float _attackCooldown = 0.0f;
    float _attackPoseTime = 0.0f;
    BlinkFlash _damageFlash;
    float _aggroRemaining = 0.0f;
    const char* _lastCountedPlayerAttack = nullptr;
    unsigned int _slashesTaken = 0;
    // Hits landed on the target since aggro was last gained.
    unsigned int _hitsLanded = 0;
    bool _turnAfterPause = false;
    bool _holdingAttackPosition = false;
    bool _wasChasing = false;
    bool _aggro = false;
    bool _canAttack = false;
    bool _defeated = false;
    // Reused by gatherTerrain so the per-tick spatial query does not allocate.
    mutable std::vector<GameObject*> _supportCandidates;
    struct TerrainTile { const Collidable* collidable; vector2 min, max; };
    mutable std::vector<TerrainTile> _terrain;

    void gatherTerrain(vector2 min, vector2 max) const;
    bool surfaceAt(float x, float minY, float maxY, bool solidOnly, float& surface) const;
    bool supportAt(float x, float footY, float above, float below, float& support) const;
    bool isFloor(const Collidable* terrain) const;
    bool followGround(float fromX, float toX, float& surface) const;
    bool groundContinues(float x, float footY, int direction, float distance) const;
    void animate(const char* name);
    void clearAggro();

protected:
    void handleCollisionContact(const CollisionContact& contact) override;

public:
    Boar(ObjectManager& world, Character& target, vector2 nearSpawn);
    void reset();
    void update(float time) override;
    void updateCombat(float time);
    bool shouldCollideWith(const GameObject& other) const override;
    bool shouldResolvePhysicsWith(const GameObject& other) const override;
};
