#pragma once

#include "../GameObject.h"
#include "../Square.h"

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
    float _damageFlashRemaining = 0.0f;
    float _damageFlashPhase = 0.0f;
    const char* _lastCountedPlayerAttack = nullptr;
    unsigned int _slashesTaken = 0;
    bool _turnAfterPause = false;
    bool _holdingAttackPosition = false;
    bool _wasChasing = false;
    bool _canAttack = false;
    bool _damageFlashOn = false;
    bool _defeated = false;
    // Reused by supportAt so the per-tick spatial query does not allocate.
    mutable std::vector<GameObject*> _supportCandidates;

    bool supportAt(float x, float footY, float above, float below, float& support) const;
    void animate(const char* name);
    void setDamageFlash(bool enabled);
    void startDamageFlash();
    void updateDamageFlash(float time);

protected:
    void handleCollisionContact(const CollisionContact& contact) override;

public:
    Boar(ObjectManager& world, Character& target, vector2 nearSpawn);
    void reset();
    void update(float time) override;
    void updateCombat(float time);
    bool shouldCollideWith(const GameObject& other) const override;
};
