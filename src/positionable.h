#pragma once

#include "Types.h"

class Positionable {

protected:

   vector2 _position;

public:

   Positionable(void) :
      _position(vector2(0, 0)) {}

   vector2 GetPosition(void) const { return _position; }
   void SetPosition(float x, float y) { _position = vector2(x, y); }
   void SetPosition(vector2 position) { _position = position; }
};