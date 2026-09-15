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
    Square _body{vector2(-18.0f, -8.0f), 36.0f, 22.0f};
    vector2 _spawn;
    int _direction = -1;
    float _pause = 0.6f;
    float _damageCooldown = 0.0f;
    bool _defeated = false;

    bool supportAt(float x, float footY, float above, float below, float& support) const;
    void animate(const char* name);

public:
    Boar(ObjectManager& world, Character& target, vector2 nearSpawn);
    void reset();
    void update(float time) override;
    void updateCombat(float time);
    bool shouldCollideWith(const GameObject& other) const override;
};
