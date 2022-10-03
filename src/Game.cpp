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
   }
   else {
      Engine2D::Quit();
   }
}

// Adding a sort of garbage collector
void Game::End(void)
{
   while (!this->empty())
   {
      void *gameState = this->top();
      this->pop();
      delete reinterpret_cast<GameState*>(gameState);
   }
}