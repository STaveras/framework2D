// File: Engine2D.h
#pragma once
#include "ISingleton.h"
#include "EventSystem.h"
#include "Timer.h"
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
   EventSystem _EventSystem; // Move to the CPP?
   Timer _Timer; // Move to the CPP?

public:
   Engine2D(void);
   ~Engine2D(void) {}

   bool HasQuit(void) const { return _hasQuit; }

   Game* GetGame(void) const { return _pGame; }
   void SetGame(Game* game) { _pGame = game; }

   EventSystem* GetEventSystem(void) { return &_EventSystem; }
   Timer* GetTimer(void) { return &_Timer; }

   IRenderer* GetRenderer(void) { return _pRenderer; }
   void SetRenderer(IRenderer* renderer) { _pRenderer = renderer; }

   IInput* GetInput(void) { return _pInput; } // Add support for multiple devices...?
   void SetInputInterface(IInput* pInput) { _pInput = pInput; }

   void Initialize(void);
   void Update(void);
   void Shutdown(void);
   // Pause? We should be able to "fast forward" and "rewind" as well... Should this be implemented by the 'game' (application)?
   void Quit(void) { _hasQuit = true; }
};
// Author: Stanley Taveras