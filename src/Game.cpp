#include "Game.h"
#include "GameState.h"
#include "Timer.h"

void Game::begin(void)
{
	namespace fs = std::filesystem;
	fs::path current = fs::current_path();

	for (auto& p : fs::recursive_directory_iterator(current)) {
		if (p.is_regular_file() && p.path().filename() == "title") {
			std::string subString = p.path().parent_path().string();

			// Read title contents
			std::ifstream titleFile(p.path());
			std::string titleStr;
			std::getline(titleFile, titleStr);

			// Set window title
			Renderer::mainWindow->setWindowTitle(titleStr.c_str());  // Ensure SetTitle is accessible here
		}
	}
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