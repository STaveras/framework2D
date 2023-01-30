#include "Game.h"
#include "GameState.h"
#include "Timer.h"

void Game::begin(void)
{
   throw std::runtime_error("Game::Begin() unimplemented.");
}

void Game::update(Timer* timer)
{
   if (!this->empty()) {

      if (timer) {

         for (Player* player : _players) {
            player->update((float)timer->getDeltaTime());
         }

         this->top()->onExecute((float)timer->getDeltaTime());
      }
      else
         this->top()->onExecute();
   }
   else {
      Engine2D::quit();
   }
}

void Game::end(void)
{
   this->clear();
}