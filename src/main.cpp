// main.cpp
#include "Engine2D.h"
#include "Camera.h"
#include "FileSystem.h"
#include "Input.h"
#include "InputEvent.h"
#include "Game.h"
#include "GameState.h"
#include "GameObject.h"
#include "System.h"
#include "Renderer.h"
#include "Window.h"

#include <iostream>

// TODO: Put this in a DLL and have loader functions to search for "game" library files
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

bool checkForVKCommand(const char* lpCmdLine) {

   if (strstr(lpCmdLine, "-vk")) {
      return true;
   }
   return false;
}

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

   RenderingInterface* pRenderer = nullptr;
   InputInterface* pInput = nullptr;

#ifdef _WIN32
   if (!checkForVKCommand(lpCmdLine)) {
      window.Initialize(hInstance, lpCmdLine);
      pInput = (DirectInput*)Input::CreateDirectInputInterface(window.GetHWND(), hInstance);
      pRenderer = (RendererDX*)Renderer::CreateDXRenderer(window.GetHWND(), GLOBAL_WIDTH, GLOBAL_HEIGHT, false, false);
   }
   else
#endif 
   {
      window.Initialize();
      pInput = (IInput*)Input::CreateInputInterface(&window); // right now would not work in windows
      pRenderer = (RendererVK*)Renderer::CreateVKRenderer(&window);
   }

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