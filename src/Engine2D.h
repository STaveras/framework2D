// File: Engine2D.h
#pragma once
#include "Types.h"
#include "ISingleton.h"
#include "EventSystem.h"
#include "Timer.h"
#ifdef _DEBUG
#include "Debug.h"
#endif
class Game;
class IInput;
class IRenderer;
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

   static bool HasQuit(void) { return Engine2D::getInstance()->_hasQuit; }

   static Game* GetGame(void) { return Engine2D::getInstance()->_pGame; }
   static void SetGame(Game* game) { Engine2D::getInstance()->_pGame = game; }

   static EventSystem* getEventSystem(void);
   static Timer* getTimer(void);

   static IRenderer* GetRenderer(void);
   static void SetRenderer(IRenderer* renderer) { Engine2D::getInstance()->_pRenderer = renderer; }

   static IInput* GetInput(void) { return Engine2D::getInstance()->_pInput; } // Add support for multiple devices...?
   static void SetInputInterface(IInput* pInput) { Engine2D::getInstance()->_pInput = pInput; }

   void Initialize(void);
   void Update(void);
   void Shutdown(void);

   // Pause? We should be able to "fast forward" and "rewind" as well... Should this be implemented by the 'game' (application)?
   static void Quit(void) { Engine2D::getInstance()->_hasQuit = true; }
};
// Author: Stanley Taveras