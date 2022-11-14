// Game.h
// Is the "Application" built with this framework

#pragma once

#include "ProgramStack.h"
#include "Player.h"
#include "Factory.h"

#include <vector>

class Game : public ProgramStack // A game should probably CONTAIN a program stack and not BE one (?)
{
   friend class Engine2D;

protected:
   Factory<Player> _players;

public:
   virtual void Begin(void) = 0;
   virtual void update(class Timer* timer);
   virtual void End(void) = 0;
};