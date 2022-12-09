// Game.h
// Is the "Application" built with this framework

#pragma once

#include "ProgramStack.h"
#include "Player.h"
#include "Factory.h"
#include "Engine2D.h"

#include <vector>

class Game : public ProgramStack
{
	friend class Engine2D;

protected:
	Factory<Player> _players;

public:
	
	static Factory<Player>* getPlayers(void) { return &(Engine2D::getGame()->_players); }

	static Player* getPlayerWith(GameObject* object) {
		for (unsigned int i = 0; i < Game::getPlayers()->size(); i++) {
			if (Game::getPlayers()->at(i)->getGameObject() == object) {
				return Game::getPlayers()->at(i);
			}
		}
		return NULL;
	}

	virtual void begin(void) = 0;
	virtual void update(class Timer* timer);
	virtual void end(void) = 0;
};