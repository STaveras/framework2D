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

#ifdef _WIN32

//#include <vld.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   int argc = 0;
   LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

#else
int main(int argc, char **argv)
{
   (System::checkArgumentsForDebugMode(argc, argv)) ? Debug::Mode.enable() : Debug::Mode.disable(); // ONLY TIME WE CHECK FOR THIS
   
   std::cout << "Working directory: " << FileSystem::GetWorkingDirectory() << std::endl; 
   
   System::GlobalDataPath(System::checkArgumentsForDataPath(argc, argv));

#endif

   if (Debug::Mode.isEnabled()) {
      // Check for game data
      FileSystem::ScoutDirectory(System::GlobalDataPath());
   }

   Window window = Window(GLOBAL_WIDTH, GLOBAL_HEIGHT, Engine2D::Version());

   RenderingInterface* pRenderer = nullptr;
   InputInterface* pInput = nullptr;

#ifdef _WIN32
   if (!System::checkArgumentsForVulkan(argc, argv)) {
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

#ifdef _DEBUG
   static unsigned int lastFPS = 0;
      if (DEBUGGING) {
         if (engine->getTimer()->GetElapsedTime() >= 1.0f) {
            std::cout << "FPS: " << (lastFPS + engine->getTimer()->GetFPS()) / 2 << std::endl;
            lastFPS = engine->getTimer()->GetFPS();
            engine->getTimer()->Reset();
         }
      }
#endif
   }

   engine->Shutdown();
   
   Input::DestroyInputInterface(pInput);
   Renderer::DestroyRenderer(pRenderer);

   window.Shutdown();

   return 0;
}
// Stan Taveras