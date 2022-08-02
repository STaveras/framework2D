// main.cpp
#include "Engine2D.h"
#include "Camera.h"
#include "Input.h"
#include "InputEvent.h"
#include "Game.h"
#include "GameState.h"
#include "GameObject.h"
#include "Renderer.h"
#include "Window.h"

#include <iostream>

//#include "SDSParser.h"

#include "FollowObjectOperator.h"
#include "ApplyVelocityOperator.h"
// #include "MaxVelocityOperator.h"
#include "UpdateRenderableOperator.h"
#include "UpdateBackgroundOperator.h"

#include "AnimationUtils.h"
#include "FileSystem.h"

#include "FlappyTurd.h"

// Ultimately, I want the executable to just be able to support running games without having to statically build a game
// from C++ source files. I'd like to be able to load a DLL with game classes and bundle scripts in the data folder that
// load assets, levels, and other miscellaneous data. Like a more modern MUGEN

const char* checkArgumentsForDataPath(int argc, char** argv) 
{
   if (argc > 1) {

      for (int i = 0; i < argc; i++) {
         if (!strcmp(argv[i], "--dataPath") || !strcmp(argv[i], "-D")) {
            return argv[i + 1]; // What if it's empty?
         }
      }
   }
   return DEFAULT_DATA_PATH;
}

#ifdef _WIN32

//#include <vld.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   // TODO: Process lpCmdLine for data path and/or other flags
#else
int main(int argc, char **argv)
{
   std::cout << "Working directory: " << FileSystem::GetWorkingDirectory() << std::endl;

   System::GlobalDataPath(checkArgumentsForDataPath(argc, argv));

#endif
   // Check for game data
   FileSystem::ScoutDirectory(System::GlobalDataPath());

   Window window = Window(320, 460, "Flap a Turd");

#ifdef _WIN32
   window.Initialize(hInstance, lpCmdLine);

   DirectInput *pInput = (DirectInput *)Input::CreateDirectInputInterface(window.GetHWND(), hInstance);
   RendererDX *pRenderer = (RendererDX *)Renderer::CreateDXRenderer(window.GetHWND(), GLOBAL_WIDTH, GLOBAL_HEIGHT, false, false);
#else
   window.Initialize();

   RendererVK *pRenderer = (RendererVK *)Renderer::CreateVKRenderer(&window);
   IInput *pInput = (IInput*)Input::CreateInputInterface(&window);
#endif

   Engine2D *engine = Engine2D::getInstance();
   engine->SetInputInterface(pInput);
   engine->SetRenderer(pRenderer);
   engine->SetGame(&game);
   engine->Initialize();

   while (!window.HasQuit() && !engine->HasQuit())
   {
      window.Update();
      engine->Update();
   }

   engine->Shutdown();
   
   Input::DestroyInputInterface(pInput);
   Renderer::DestroyRenderer(pRenderer);

   window.Shutdown();

   return 0;
}
// Stan Taveras