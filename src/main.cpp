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

   LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
   LPSTR* argv = new LPSTR[argc];

	for (int i = 0; i < argc; i++) {
		argv[i] = new CHAR[wcslen(argvW[i]) + 1]{ 0 };
#if _DEBUG
		OutputDebugStringW(L"\n");
		OutputDebugStringW(argvW[i]);
#endif
		WideCharToMultiByte(CP_UTF8,
			WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK,
			argvW[i], -1,
			argv[i], wcslen(argvW[i]),
			NULL, false);
#if _DEBUG
		OutputDebugStringA(argv[i]);
#endif
	}

#else
int main(int argc, char **argv)
{
#endif
   std::cout << "Working directory: " << FileSystem::GetWorkingDirectory() << std::endl;
   System::GlobalDataPath(System::checkArgumentsForDataPath(argc, argv));

#if _DEBUG
   (System::checkArgumentsForDebugMode(argc, argv)) ? Debug::Mode.enable() : Debug::Mode.disable(); // ONLY TIME WE CHECK FOR THIS

   if (Debug::Mode.isEnabled()) {
      // Check for game data
      FileSystem::ListDirectoryContents(System::GlobalDataPath());
   }
#endif

   Window window = Window(GLOBAL_WIDTH, GLOBAL_HEIGHT, Engine2D::Version());

   RenderingInterface* pRenderer = nullptr;
   InputInterface* pInput = nullptr;
   
#ifdef _WIN32
   if (!System::checkArgumentsForVulkan(argc, argv)) {

      Renderer::window = &window;

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

   pRenderer->setFullScreen(System::checkArgumentsForFullscreen(argc, argv));
   pRenderer->setVerticalSync(System::checkArgumentsForVSync(argc, argv));

   Engine2D *engine = Engine2D::getInstance();
   engine->SetInputInterface(pInput);
   engine->SetRenderer(pRenderer);
   engine->SetGame(&game);
   engine->Initialize();

   try {
      while (!window.HasQuit() && !engine->HasQuit())
      {
         // Eventually just have the engine handle this like:
         // engine->Run(); 
         window.Update();
         engine->Update();
   #ifdef _DEBUG
         static unsigned int lastFPS = 0;
         if (DEBUGGING) {
            if (engine->getTimer()->GetElapsedTime() >= 1.0f) {
               std::cout << "FPS: " << std::to_string((lastFPS + engine->getTimer()->GetFPS()) / 2) << std::endl;
               lastFPS = engine->getTimer()->GetFPS();
               engine->getTimer()->Reset();
            }
         }
   #endif
      }
   }
   catch (std::exception& e) {
      std::cout << Engine2D::getTimer()->GetTimeStamp() << ": " << e.what() << std::endl;

#ifdef _WIN32
      MessageBoxA(NULL, e.what(), "Error", MB_OK);
#else
      // TODO: GLFW message box somehow
#endif
   }

   engine->Shutdown();
   
   Input::DestroyInputInterface(pInput);
   Renderer::DestroyRenderer(pRenderer);

   window.Shutdown();

#if _WIN32
   for (int i = 0; i < argc; i++) {
      delete argv[i];
   }
   delete [] argv;
#endif

   return 0;
}