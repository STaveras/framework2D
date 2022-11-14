// File: Engine2D.h
// Mostly a nexus point of subsystems that support an object graph and a collection of "operators" to apply effects to the graph (e.g. physics, rendering, etc.)) 
#pragma once
#include "Types.h"
#include "ISingleton.h"
#include "EventSystem.h"
#include "Timer.h"
#include "System.h"
#include "Renderer.h"
#ifdef _DEBUG
#include "Debug.h"
#endif
class Game;
class IInput;
class Engine2D : public ISingleton<Engine2D>
{
   friend ISingleton<Engine2D>;

   bool _hasQuit;
   Game* _pGame;
   IInput* _pInput;
   IRenderer* _pRenderer;

public:
   Engine2D(void);
   ~Engine2D(void) {}

   static bool hasQuit(void) { return Engine2D::getInstance()->_hasQuit; }

   static Game* getGame(void) { return Engine2D::getInstance()->_pGame; }
   static void setGame(Game* game) { Engine2D::getInstance()->_pGame = game; }

   static EventSystem* getEventSystem(void);
   static Timer* getTimer(void);

   static IRenderer* getRenderer(void);
   static void setRenderer(IRenderer* renderer) { Engine2D::getInstance()->_pRenderer = renderer; }

   static IInput* getInput(void) { return Engine2D::getInstance()->_pInput; } // Add support for multiple devices...?
   static void setInputInterface(IInput* pInput) { Engine2D::getInstance()->_pInput = pInput; }

   void initialize(void);
   void update(void);
   void shutdown(void);

   // pause? We should be able to "fast forward" and "rewind" as well... Should this be implemented by the 'game' (application)?
   static void quit(void) { Engine2D::getInstance()->_hasQuit = true; }
   static const char* version(void) { return FRAMEWORK_VERSION; }
};
// Author: Stanley Taveras