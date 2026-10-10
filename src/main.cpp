// main.cpp
#include "Engine2D.h"
#include "Camera.h"
#include "FileSystem.h"
#include "Input.h"
#include "Game.h"
#include "GameState.h"
#include "GameObject.h"
#include "System.h"
#include "Renderer.h"
#include "Window.h"
#include "RuntimeProfile.h"
#include "FramePacer.h"

#include <iostream>
#include <chrono>
#include <algorithm>
#include <string>
#include <vector>

// TODO: Put this in a DLL and have loader functions to search for "game" library files
#include "FantasySideScroller/FantasySideScroller.h"

// Ultimately, I want the executable to just be able to support running games without having to statically build a game
// from C++ source files. I'd like to be able to load a DLL with game classes and bundle scripts in the data folder that
// load assets, levels, and other miscellaneous data. Like a more modern MUGEN 

#if defined(_WIN32) && !defined(_DEBUG)

// One UTF-16 command-line argument as UTF-8, for System::checkArguments*.
static std::string narrowArgument(const wchar_t* argument)
{
   // CP_UTF8 accepts no conversion flags; a size query first, then the conversion.
   const int size = WideCharToMultiByte(CP_UTF8, 0, argument, -1, nullptr, 0, nullptr, nullptr);
   if (size <= 1) {
      return std::string();
   }
   std::string narrow((size_t)size, '\0');
   WideCharToMultiByte(CP_UTF8, 0, argument, -1, &narrow[0], size, nullptr, nullptr);
   narrow.pop_back(); // the terminator the conversion wrote
   return narrow;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
   // The strings live until WinMain returns; argv points into them.
   std::vector<std::string> arguments;
   int argc = 0;
   if (LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc)) {
      for (int i = 0; i < argc; i++) {
         arguments.push_back(narrowArgument(argvW[i]));
      }
      LocalFree(argvW);
   }
   std::vector<const char*> argumentPointers;
   for (const std::string& argument : arguments) {
      argumentPointers.push_back(argument.c_str());
   }
   argc = (int)argumentPointers.size();
   const char** argv = argumentPointers.data();
#else

#if defined(_WIN32) && defined(_DEBUG)

#if defined(FRAMEWORK_ENABLE_VLD)
#include <vld.h>
#define FRAMEWORK_HAS_VLD 1
#else
#define FRAMEWORK_HAS_VLD 0
#endif

HINSTANCE hInstance = GetModuleHandle(NULL);
LPSTR lpCmdLine = GetCommandLine();

#endif
int main(int argc, const char *argv[])
{
#endif
   std::cout << "Working directory: " << FileSystem::GetWorkingDirectory() << std::endl;

   System::GlobalDataPath(System::checkArgumentsForDataPath(argc, argv));

#if defined(_WIN32) && defined(_DEBUG) && FRAMEWORK_HAS_VLD
   // Allocation stack tracing is expensive; opt in independently of overlays.
   Debug::dbgMemory = System::checkEnvironmentFlag("AUTO_MEMORY_DEBUG");
   if (Debug::dbgMemory) {
      VLDGlobalEnable();
   }
   else {
      VLDGlobalDisable();
   }
#endif

   const bool enableDebug = System::checkArgumentsForDebugMode(argc, argv);
   enableDebug ? Debug::Mode.enable() : Debug::Mode.disable(); // set runtime debug mode
   if (enableDebug) {
      Debug::dbgCollision = System::checkArgumentsForCollisionDebug(argc, argv);
      Debug::dbgObjects = System::checkArgumentsForObjectDebug(argc, argv);
      Debug::dbgTiles = System::checkArgumentsForTileDebug(argc, argv);
   }

   const bool deterministicMode =
      System::checkArgumentsForDeterministic(argc, argv) ||
      System::checkEnvironmentFlag("AUTO_DETERMINISTIC");

   double fixedDtMs = System::checkArgumentsForFixedDtMs(argc, argv, 0.0);
   if (fixedDtMs <= 0.0) {
      fixedDtMs = System::checkEnvironmentDouble("AUTO_FIXED_DT_MS", 16.6667);
   }
   if (fixedDtMs <= 0.0) {
      fixedDtMs = 16.6667;
   }

#if _DEBUG
   if (Debug::Mode.isEnabled()) {
      // Check for game data
      FileSystem::ListDirectoryContents(System::GlobalDataPath());
   }
   // sleep is in milliseconds for Windows, seconds for others!
   // sleep(1000);
#endif

   Window window = Window(GLOBAL_WIDTH, GLOBAL_HEIGHT, Engine2D::version());

   Renderer::mainWindow = &window;

   RenderingInterface* pRenderer = nullptr;
   std::unique_ptr<IInput> input;

   const bool useVulkan = System::checkArgumentsForVulkan(argc, argv);
   const bool useOpenGL = System::checkArgumentsForOpenGL(argc, argv);
   const bool useStaticBackground = System::checkArgumentsForStaticBackground(argc, argv);

   if (useOpenGL) {
      window.initialize(Window::ClientAPI::OpenGL);
      input = Input::createInputInterface(&window);
      pRenderer = (RenderingInterface*)(RendererGL*)Renderer::createGLRenderer(&window);
   }
   else if (useVulkan) {
      window.initialize(Window::ClientAPI::None, true);
      input = Input::createInputInterface(&window);
      pRenderer = (RenderingInterface*)(RendererVK*)Renderer::createVKRenderer(&window);
   }
#if _WIN32
   else {
      window.initialize(hInstance, lpCmdLine);
      input = Input::createDirectInputInterface(window.getHWND(), hInstance); 
      pRenderer = (RendererDX*)Renderer::createDXRenderer(window.getHWND(), GLOBAL_WIDTH, GLOBAL_HEIGHT, false, false);
   }
#elif defined(__linux__)
   else {
      window.initialize(Window::ClientAPI::OpenGL);
      input = Input::createInputInterface(&window);
      pRenderer = (RenderingInterface*)(RendererGL*)Renderer::createGLRenderer(&window);
   }
#elif __APPLE__
   else if (System::checkArgumentsForMetal(argc, argv)) {
      // Metal draws into a CAMetalLayer, so the window gets no OpenGL context
      window.initialize(Window::ClientAPI::None);
      input = Input::createInputInterface(&window);
      pRenderer = (RenderingInterface*)(RendererMTL*)Renderer::createMTLRenderer(&window);
   }
   else {
      window.initialize(Window::ClientAPI::OpenGL);
      input = Input::createInputInterface(&window);
      pRenderer = (RenderingInterface*)(RendererGL*)Renderer::createGLRenderer(&window);
   }
#endif

   if (pRenderer) {
      pRenderer->setBackgroundStatic(useStaticBackground);
   }

   // We need to only call setFullscreen or setVericalSync when the command line argument for either is present
   if (System::checkArgumentsForFullscreen(argc, argv))
      pRenderer->setFullScreen(true);
   
   if (System::checkArgumentsForVSync(argc, argv))
      pRenderer->setVerticalSync(true);

   // Late input sampling: on by default with VSync. AUTO_INPUT_PACING=0/1 overrides;
   // AUTO_SIMULATE_VSYNC=1 paces against a virtual display (headless measurements).
   {
      FramePacer::Config pacing;
      pacing.simulateVsync = System::checkEnvironmentFlag("AUTO_SIMULATE_VSYNC");
      const double pacingSetting = System::checkEnvironmentDouble("AUTO_INPUT_PACING", -1.0);
      pacing.enabled = (pacingSetting < 0.0)
         ? (pRenderer && pRenderer->verticalSyncEnabled()) || pacing.simulateVsync
         : pacingSetting > 0.0;
      double refreshHz = System::checkEnvironmentDouble("AUTO_REFRESH_HZ", 0.0);
      if (refreshHz <= 0.0 && window.getUnderlyingWindow()) {
         GLFWmonitor* monitor = glfwGetWindowMonitor(window.getUnderlyingWindow());
         if (!monitor) monitor = glfwGetPrimaryMonitor();
         const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
         if (mode) refreshHz = mode->refreshRate;
      }
      pacing.refreshHz = (refreshHz > 0.0) ? refreshHz : 60.0;
      pacing.marginMs = System::checkEnvironmentDouble("AUTO_INPUT_PACING_MARGIN_MS", pacing.marginMs);
      FramePacer::configure(pacing);
   }

   Engine2D *engine = Engine2D::getInstance();
   engine->setDeterministicMode(deterministicMode);
   engine->setFixedDeltaSeconds(fixedDtMs / 1000.0);
   engine->setRenderInterpolation(System::checkEnvironmentDouble("AUTO_RENDER_INTERPOLATION", 1.0) > 0.0);
   engine->setRenderInterpolationSnapDistance((float)System::checkEnvironmentDouble("AUTO_RENDER_INTERPOLATION_SNAP", 128.0));
   engine->setInputInterface(input.get());
   engine->setRenderer(pRenderer);
   engine->setGame(&game);
   engine->initialize();

   // Opt-in, bounded benchmark: exclude startup and warm up for one second.
   const double benchmarkSeconds = System::checkEnvironmentDouble("AUTO_BENCHMARK_SECONDS", 0.0);
   const bool profileEnabled = System::checkEnvironmentFlag("AUTO_PROFILE");
   using BenchmarkClock = std::chrono::steady_clock;
   auto benchmarkStart = BenchmarkClock::now();
   bool benchmarkNeedsStart = true;
   std::vector<double> frameTimes;
   if (benchmarkSeconds > 0.0) frameTimes.reserve(10000);

   try {
      do 
      {
         // Eventually just have the engine handle this like:
         // engine->Run(); 
         const auto frameStart = BenchmarkClock::now();
         RuntimeProfile::active = profileEnabled && !benchmarkNeedsStart &&
             std::chrono::duration<double>(frameStart - benchmarkStart).count() >= 1.0;
         FramePacer::waitForInputDeadline();
         window.update();
         FramePacer::markInputSampled();
         engine->update();

         // Compose window title with renderer and FPS info without overwriting one another
         std::string currentTitle = window.getWindowTitle();
         size_t semiColonIndex = currentTitle.find_first_of(';');
         std::string baseTitle = currentTitle.substr(0, (semiColonIndex != std::string::npos) ? semiColonIndex : currentTitle.size());

         std::string suffix;
         
         if (Debug::Mode.isEnabled()) {
             suffix += "Renderer: " + RENDERER_API_TYPE::toString(Renderer::get()->renderingAPI());
         }

         if (System::checkArgumentsForFPSCounter(argc, argv)) 
         {
#ifdef _DEBUG
            static Timer timer; timer.update();

            std::string framesPerSecond = "FPS: ";

            if (DEBUGGING) {

                if (timer.getElapsedTime() >= 1.0f) {

                    framesPerSecond += std::to_string(engine->getTimer()->getFPS()) + "\n";
                    DEBUG_MSG(framesPerSecond.c_str());
                    timer.reset();
                }
            }
#endif
            // Append FPS to the suffix (preserve renderer info if present)
            if (!suffix.empty()) suffix += "; ";
            suffix += "FPS: " + std::to_string(engine->getTimer()->getFPS());
         }

         if (!suffix.empty()) {
             std::string newTitle = baseTitle + "; " + suffix;
             window.setWindowTitle(newTitle.c_str());
         }

         if (benchmarkSeconds > 0.0 || profileEnabled) {
            const auto now = BenchmarkClock::now();
            // The initial update can load the level through queued events.
            // Start warm-up after that frame, not before deferred loading.
            if (benchmarkNeedsStart) {
               benchmarkStart = now;
               benchmarkNeedsStart = false;
            }
            const double elapsed = std::chrono::duration<double>(now - benchmarkStart).count();
            if (benchmarkSeconds > 0.0 && std::chrono::duration<double>(frameStart - benchmarkStart).count() >= 1.0) {
               frameTimes.push_back(std::chrono::duration<double, std::milli>(now - frameStart).count());
            }
            if (benchmarkSeconds > 0.0 && elapsed >= benchmarkSeconds + 1.0) break;
         }
      } while (!window.hasQuit() && !engine->hasQuit());
   }
   catch (std::exception& e) {
      std::cout << Engine2D::getTimer()->getTimeStamp() << ": " << e.what() << std::endl;

#ifdef _WIN32
      MessageBoxA(NULL, e.what(), "Error", MB_OK);
#else
      // TODO: GLFW message box somehow
#endif
   }

   if (!frameTimes.empty()) {
      double total = 0.0;
      size_t overBudget = 0;
      for (double ms : frameTimes) { total += ms; if (ms > 1000.0 / 60.0) ++overBudget; }
      std::sort(frameTimes.begin(), frameTimes.end());
      std::cout << "BENCHMARK frames=" << frameTimes.size()
                << " fps=" << 1000.0 * frameTimes.size() / total
                << " mean_ms=" << total / frameTimes.size()
                << " p95_ms=" << frameTimes[(frameTimes.size() - 1) * 95 / 100]
                << " p99_ms=" << frameTimes[(frameTimes.size() - 1) * 99 / 100]
                << " over_16.67ms=" << overBudget << std::endl;
   }
   RuntimeProfile::active = false;
   if (profileEnabled) {
      RuntimeProfile::report(std::cout);
      FramePacer::report(std::cout);
   }
   engine->shutdown();
   
   input.reset();
   Renderer::destroyRenderer(pRenderer);

   window.shutdown();

   return 0;
}
