#pragma once

#include "Types.h"

class positionable {

protected:

   vector2 m_Position;

public:

   positionable(void) :
      m_Position(vector2(0, 0)) {}

   vector2 GetPosition(void) const { return m_Position; }
   void SetPosition(vector2 position) { m_Position = position; }

};