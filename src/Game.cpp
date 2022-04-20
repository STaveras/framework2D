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
         timer = new Timer();
         this->Update(new Timer());
         delete timer;
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