// File: Engine2D.h
// Mostly a nexus point of subsystems that support an object graph and a collection of "operators" to apply effects to the graph (e.g. physics, rendering, etc.)) 
#pragma once
#include "Types.h"
#include "ISingleton.h"
#include "EventSystem.h"
#include "Timer.h"
#include "System.h"
#include "Renderer.h"
#include "Debug.h"

#include <cstdint>

class Game;
class IInput;
class Engine2D : public ISingleton<Engine2D>
{
   friend ISingleton<Engine2D>;

   bool _hasQuit;
   Game* _game;
   IInput* _input;
   IRenderer* _renderer;
   bool _deterministicMode;
   double _fixedDeltaSeconds;
   double _frameAccumulatorSeconds;
   uint64_t _simulationTick;
   double _simulationElapsedSeconds;

public:
   Engine2D(void);
   ~Engine2D(void) {}

   static bool hasQuit(void) { return Engine2D::getInstance()->_hasQuit; }

   static Game* getGame(void) { return Engine2D::getInstance()->_game; }
   static void setGame(Game* game) { Engine2D::getInstance()->_game = game; }

   static EventSystem* getEventSystem(void);
   static Timer* getTimer(void);

   static IRenderer* getRenderer(void);
   static void setRenderer(IRenderer* renderer) { 
      Engine2D::getInstance()->_renderer = renderer;
   }

   static IInput* getInput(void) { return Engine2D::getInstance()->_input; } // Add support for multiple devices...?
   static void setInputInterface(IInput* pInput) { Engine2D::getInstance()->_input = pInput; }
   static bool isDeterministicMode(void) { return Engine2D::getInstance()->_deterministicMode; }
   static void setDeterministicMode(bool enabled) { Engine2D::getInstance()->_deterministicMode = enabled; }
   static double getFixedDeltaSeconds(void) { return Engine2D::getInstance()->_fixedDeltaSeconds; }
   static void setFixedDeltaSeconds(double seconds);
   static uint64_t getSimulationTick(void) { return Engine2D::getInstance()->_simulationTick; }
   static double getSimulationElapsedSeconds(void) { return Engine2D::getInstance()->_simulationElapsedSeconds; }

   void initialize(void);
   void update(void);
   void shutdown(void);

   // pause? We should be able to "fast forward" and "rewind" as well... Should this be implemented by the 'game' (application)?
   static void quit(void) { Engine2D::getInstance()->_hasQuit = true; }
   static const char* version(void) { return FRAMEWORK_VERSION; }
};
