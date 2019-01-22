#pragma once

#include "Types.h"

class Positionable {

protected:
   vector2 _position;

public:
   Positionable(void) :
      _position(vector2(0, 0)) {}
   Positionable(float x, float y) :
      _position(x, y) {}
   Positionable(vector2 position) :
      _position(position) {}

   vector2 getPosition(void) const { return _position; }
   void setPosition(float x, float y) { _position.x = x; _position.y = y; }
   void setPosition(vector2 position) { _position = position; }
};

// This is a weird one
class ICyclical
{
public:
   virtual void update(float time) = 0;
};

// For actually making physics a thing in this engine sometime in the future...
class Physical : public Positionable, public ICyclical {

protected:
   float _mass;
   float _rotation;
   vector2 _velocity;

public:
   Physical(void) :
      _mass(0.0f),
      _rotation(0.0f),
      _velocity(0.0f, 0.0f){}

   float GetRotation(void) const { return _rotation; }
   vector2 GetVelocity(void) const { return _velocity; }
   void SetRotation(float rotation) { _rotation = rotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)
   void SetVelocity(vector2 velocity) { _velocity = velocity; }

   // HMMMMMMM (implies that this engine should have had, i dunno, actual physics)
   void AddImpulse(vector2 direction, double force) {
      _velocity += (direction * (float)force);
   }
   
   void update(float time) {
      _position += _velocity * time;
   }
};