#pragma once
#include "ObjectManager.h"
#include "ProgramStack.h"
#include "Player.h"
class Timer;
class Game : public ProgramStack // A game should probably CONTAIN a program stack and not BE one (?)
{
   friend class Engine2D;

protected:
   Player* _Players;

public:
   Player* GetPlayer(unsigned int index) { return &(_Players[index]); }
   Player* GetPlayers(void) const { return _Players; }

   virtual void Begin(void) = 0;
   virtual void Update(Timer* timer);
   virtual void End(void) = 0;

   // virtual ~Game(void) { delete[] _Players; }
};