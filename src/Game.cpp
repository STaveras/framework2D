#include "Game.h"
#include "GameState.h"
#include "IInput.h"
#include "Timer.h"
#include "RuntimeProfile.h"

void Game::begin(void)
{
	// The data folder's "title" file names the game (it is also how
	// System::GlobalDataPath finds that folder).
	std::ifstream titleFile(std::filesystem::path(System::GlobalDataPath()) / "title");
	std::string titleStr;
	if (std::getline(titleFile, titleStr) && Renderer::mainWindow) {
		Renderer::mainWindow->setWindowTitle(titleStr.c_str());
	}

	_inputManager.initialize(Engine2D::getInput());
}

void Game::update(Timer* timer)
{
   if (!this->empty()) {
      const float time = timer ? (float)timer->getDeltaTime() : 0.0f;
      {
         RuntimeProfile::Scope profile(RuntimeProfile::Region::Input);
         _inputManager.update(time);
      }

      if (timer)
         this->top()->onExecute(time);
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
   _inputManager.shutdown();
}