// main.cpp
#include <Windows.h>

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

// TODO: Put this in a DLL and have loader functions to search for "game" library files
#include "FlappyTurd.h"

#ifdef _WIN32

//#include <vld.h>

bool checkForVKCommand(const char* lpCmdLine) {

   if (strstr(lpCmdLine, "-vk")) {
      return true;
   }
   return false;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(int argc, char **argv)
#endif
{
   Window window = Window(320, 460, "Flap a Turd");
   RenderingInterface* pRenderer = nullptr;
   InputInterface* pInput = nullptr;
#ifdef _WIN32
   if (checkForVKCommand(lpCmdLine))
   {
      window.Initialize();
      pInput = nullptr; // right now would not work in windows
      pRenderer = (RendererVK*)Renderer::CreateVKRenderer(&window);
   }
   else
#endif
   {
      window.Initialize(hInstance, lpCmdLine);
      pInput = (DirectInput*)Input::CreateDirectInputInterface(window.GetHWND(), hInstance);
      pRenderer = (RendererDX*)Renderer::CreateDXRenderer(window.GetHWND(), GLOBAL_WIDTH, GLOBAL_HEIGHT, false, false);
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