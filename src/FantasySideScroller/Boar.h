#pragma once

#include "../GameObject.h"
#include "../Square.h"

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
    float _pause = 0.6f;
    float _damageCooldown = 0.0f;
    // Prevent a chase target on the far side of a wall from immediately
    // steering the boar back into the same contact on the next frame.
    float _wallTurnCooldown = 0.0f;
    bool _defeated = false;

    bool supportAt(float x, float footY, float above, float below, float& support) const;
    void animate(const char* name);

protected:
    void handleCollisionContact(const CollisionContact& contact) override;

public:
    Boar(ObjectManager& world, Character& target, vector2 nearSpawn);
    void reset();
    void update(float time) override;
    void updateCombat(float time);
    bool shouldCollideWith(const GameObject& other) const override;
};
