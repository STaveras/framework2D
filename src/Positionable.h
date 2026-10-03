#pragma once

#include "Types.h"

#ifndef _POSITIONABLE_H_
#define _POSITIONABLE_H_

// Something with a 2D position. setPosition always ends in onPositionChanged,
// which subclasses override to keep dependent state (renderables, colliders,
// cached vertices) in step with the position.
class Positionable {
protected:
   vector2 _position;

   // Called after every setPosition, including when the value didn't change,
   // so it can double as a "refresh from position" call.
   virtual void onPositionChanged(vector2 previous, vector2 current) { (void)previous; (void)current; }

public:
   Positionable(void) :
      _position(vector2(0, 0)) {}
   Positionable(float x, float y) :
      _position(x, y) {}
   Positionable(vector2 position) :
      _position(position) {}

   virtual ~Positionable(void) = default;

   vector2 getPosition(void) const { return _position; }

   void setPosition(vector2 position)
   {
      const vector2 previous = _position;
      _position = position;
      this->onPositionChanged(previous, _position);
   }
   void setPosition(float x, float y) { this->setPosition(vector2(x, y)); }

   // Moves only this object, without running onPositionChanged.
   void setPositionSilently(vector2 position) { _position = position; }
};

#endif // _POSITIONABLE_H_
