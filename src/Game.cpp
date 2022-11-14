#include "Game.h"
#include "GameState.h"
#include "Timer.h"

void Game::Begin(void)
{
   throw std::runtime_error("Game::Begin() unimplemented.");
}

void Game::update(Timer* timer)
{
   if (!this->empty()) {

      if (timer) {
         this->top()->onExecute((float)timer->GetDeltaTime());
      }
      else
         this->top()->onExecute();
   }
   else {
      Engine2D::quit();
   }
}

// We can't assume that new GameStates are allocated on the heap
void Game::End(void)
{
   throw std::runtime_error("Game::End() unimplemented.");
}