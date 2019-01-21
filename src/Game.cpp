#include "Game.h"
#include "GameState.h"
#include "Timer.h"

void Game::Begin(void)
{
   throw "Game::Begin() unimplemented.";
}

void Game::Update(Timer* timer)
{
   if (!this->empty()) {
      if (timer) {
         ProgramStack::update((float)timer->GetDeltaTime());
      }
      else {
         // TODO: Create a timer
      }
   }
   else {
      Engine2D::Quit();
   }
}

void Game::End(void)
{
   throw "Game::End() unimplemented.";
}