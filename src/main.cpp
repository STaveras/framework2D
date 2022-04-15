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

#include "FlappyTurd.h"

#ifdef _WIN32

//#include <vld.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char **argv)
#endif
{
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