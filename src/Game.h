#pragma once
#include "ObjectManager.h"
#include "ProgramStack.h"
#include "Player.h"
class Timer;
class Game : public ProgramStack
{
protected:
	Player* _Players;

public:
	Player* GetPlayer(unsigned int index) { return &(_Players[index]); }
	Player* GetPlayers(void) const { return _Players; }

	virtual void Begin(void)=0;
	virtual void Update(Timer* timer);
	virtual void End(void)=0;
};