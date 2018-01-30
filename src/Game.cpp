#include "Game.h"
#include "Timer.h"

void Game::Begin(void)
{
   throw "Game::Begin() unimplemented.";
}

void Game::Update(Timer* timer)
{
   if (timer) ProgramStack::update((float)timer->GetDeltaTime());
}

void Game::End(void)
{
   throw "Game::End() unimplemented.";
}