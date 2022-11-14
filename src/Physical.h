#pragma once

#include "Collidable.h"
#include "Cyclable.h"

// For actually making physics a thing in this "engine" sometime in the future...
class Physical : public Collidable, public Cyclable {

protected:
    float _mass;
    float _rotation;
    vector2 _velocity;

public:
    Physical(void) :
        _mass(0.0f),
        _rotation(0.0f),
        _velocity(0.0f, 0.0f) {}

    float getRotation(void) const { return _rotation; }
    void setRotation(float rotation) { _rotation = rotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)

    vector2 getVelocity(void) const { return _velocity; }
    void setVelocity(vector2 velocity) { _velocity = velocity; }

    // HMMMMMMM (implies that this engine should have had, i dunno, actual physics)
    virtual void addImpulse(vector2 direction, double force) {
        _velocity += (direction * (float)force);
    }

    virtual void update(float time) {
        _position += _velocity * time;
    }
};
