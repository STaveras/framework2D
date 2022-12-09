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
#include "FantasySideScroller.h"

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
      FileSystem::ScoutDirectory(System::GlobalDataPath());
   }
#endif

   Window window = Window(GLOBAL_WIDTH, GLOBAL_HEIGHT, Engine2D::version());

   RenderingInterface* pRenderer = nullptr;
   InputInterface* pInput = nullptr;
   
#ifdef _WIN32
   if (!System::checkArgumentsForVulkan(argc, argv)) {

      Renderer::window = &window;

      window.initialize(hInstance, lpCmdLine);
      pInput = (DirectInput*)Input::CreateDirectInputInterface(window.getHWND(), hInstance); 
      pRenderer = (RendererDX*)Renderer::CreateDXRenderer(window.getHWND(), GLOBAL_WIDTH, GLOBAL_HEIGHT, false, false);
   }
   else
#endif 
   {
      window.initialize();
      pInput = (IInput*)Input::CreateInputInterface(&window); // right now would not work in windows
      pRenderer = (RendererVK*)Renderer::CreateVKRenderer(&window); 
   }

   // We need to only call setFullscreen or setVericalSync when the command line argument for either is present
   if (System::checkArgumentsForFullscreen(argc, argv))
      pRenderer->setFullScreen(true);
   
   if (System::checkArgumentsForVSync(argc, argv))
      pRenderer->setVerticalSync(true);

   Engine2D *engine = Engine2D::getInstance();
   engine->setInputInterface(pInput);
   engine->setRenderer(pRenderer);
   engine->setGame(&game);
   engine->initialize();

   try {
      do 
      {
         // Eventually just have the engine handle this like:
         // engine->Run(); 
         window.update();
         engine->update();

   #ifdef _DEBUG
         static unsigned int lastFPS = 0;
         if (DEBUGGING) {
            if (engine->getTimer()->getElapsedTime() >= 1.0f) {

               std::string framesPerSecond = "FPS: " + std::to_string(/*(lastFPS + */engine->getTimer()->getFPS()/* / 2)*/) + "\n";
               std::cout << framesPerSecond.c_str();
               lastFPS = engine->getTimer()->getFPS();
               engine->getTimer()->reset();

               static std::string windowTitle = window.getWindowTitle();
               
               size_t semiColonIndex = windowTitle.find_first_of(';');

               std::string windowTitleSansFPS = windowTitle.substr(0, (semiColonIndex != -1) ? semiColonIndex : windowTitle.size());
               
               windowTitle = windowTitleSansFPS + "; " + framesPerSecond;

               window.setWindowTitle(windowTitle.c_str());

            }
         }
   #endif
      } while (!window.hasQuit() && !engine->hasQuit());
   }
   catch (std::exception& e) {

      std::cout << e.what() << std::endl;

#ifdef _WIN32
      MessageBoxA(NULL, e.what(), "Error", MB_OK);
#else
      // TODO: GLFW message box somehow
#endif
   }

   engine->shutdown();
   
   Input::DestroyInputInterface(pInput);
   Renderer::DestroyRenderer(pRenderer);

   window.shutdown();

#if _WIN32
   for (int i = 0; i < argc; i++) {
      delete [] argv[i];
   }
   delete [] argv;
#endif

   return 0;
}