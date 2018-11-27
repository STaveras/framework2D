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

#include "SDSParser.h"

#include "FollowObjectOperator.h"
#include "MaxVelocityOperator.h"
#include "ApplyVelocityOperator.h"
#include "UpdateRenderableOperator.h"
#include "UpdateBackgroundOperator.h"

#define FALL_FORCE 100.0f
#define FLAP_MULTIPLIER 3.33f

class FlappyTurd : public Game
{  
   class PlayState : public GameState
   {
      // (Probably should just go in GameState...?)
      Image* _background; // Lights; the set
      Camera* _camera; // Camera
      Player* _player; // Action; the actors

      // Game rules
      FollowObject _attachCamera;
      //MaxVelocityOperator _maxVelocity;
      ApplyVelocityOperator _applyVelocity;
      UpdateBackgroundOperator _updateBackground;
      UpdateRenderableOperator _updateRenderable;
      //TODO: UpdateCollidable _updateCollidable;

      class Turd : public GameObject
      {
         // Currently unused
         //class TurdState : public GameObjectState
         //{
         //public:
           // void onEnter()
           // {
           //	 //GameObjectState::onEnter();
           // }

           // void onExit()
           // {

           // }
         //};

      public:
         Turd(void)
         {
            // NOTE: All of this should ideally be in a script
            GameObjectState* falling = this->addState("Falling");
            //falling->setDirection(vector2(0.1019108280254777f, 0.1464968152866242f)); // normalized it myself ;)
            falling->setDirection(vector2(0.1f, 1.0f)); 
            falling->setForce(FALL_FORCE);
            falling->setRenderable(new Sprite("./data/images/turd0.png", 0xFFFF00FF));
            ((Image*)falling->getRenderable())->center();

            GameObjectState* rising = this->addState("Rising");
            //rising->setDirection(vector2(0.1f, -1.0f));
            rising->setExecuteTime(0.27);
            rising->setDirection(vector2(0.1f, -1.0f * FLAP_MULTIPLIER));
            rising->setForce(FALL_FORCE);
            rising->setRenderable(new Sprite("./data/images/turd1.png", 0xFFFF00FF));
            ((Image*)rising->getRenderable())->center();

            RegisterTransition("Falling", "BUTTON_PRESSED", "Rising");
            RegisterTransition("Rising", "BUTTON_PRESSED", "Rising"); // lets you chain together flaps
            RegisterTransition("Rising", "BUTTON_RELEASED", "Falling"); // stop rising when you let go of the button
         }

         ~Turd(void) 
         {
            for (unsigned int i = 0; i < _states.Size(); i++)
               delete ((GameObjectState*)_states.At(i))->getRenderable();
         }
      };

   public:

      void onEnter(void)
      {
         GameState::onEnter();

         _camera = new Camera;
         _objectManager.addObject("Camera", _camera);

         _updateBackground.useRenderList(_renderList);
         _updateBackground.setCamera(_camera);
         _updateBackground.setBackground(new Image("./data/images/bg.png"));
         _updateBackground.setMode(Background::Mode_Mirror);

         _objectManager.addObject("Turd", new Turd);
         _objectManager.pushOperator(&_updateBackground);
         _objectManager.pushOperator(&_applyVelocity);
         //_objectManager.pushOperator(&_maxVelocity);
         _objectManager.pushOperator(&_updateRenderable);
         _objectManager.pushOperator(&_attachCamera);
         _objectManager.getGameObject("Turd")->Initialize();

         _player = new Player;
         _player->setGamePad(_inputManager.CreateGamePad());
         _player->getGamePad()->addButton(VirtualButton("BUTTON", KBK_SPACE));
         _player->setGameObject(_objectManager.getGameObject("Turd"));
         _player->setup();

         _camera->SetZoom(0.5);

         _attachCamera.setSource(_camera);
         _attachCamera.follow(_objectManager.getGameObject("Turd"), true, false);
         
         Engine2D::GetRenderer()->SetCamera(_camera);
      }
      void onExecute(float time)
      {
         if (_objectManager.getGameObject("Turd")->getPosition().y < _camera->GetScreenHeight()) {
            GameState::onExecute(time);
         }
         else {// Push on 'GameOverState'
            ((FlappyTurd*)Engine2D::GetGame())->GameOver();
         }

#if _DEBUG
         static float timer = 0.0f;
         timer += time;

         if (timer >= 1.0f) {
            GameObject *debugObject = _camera;

            char debugBuffer[255];

            sprintf_s(debugBuffer, "(camera) pos: (x%f, y%f) zoom: %f\n", debugObject->getPosition().x, debugObject->getPosition().y, _camera->getZoom());
            OutputDebugString(debugBuffer);

            for (int i = 0; i < _objectManager.numObjects(); i++) {
               debugObject = _objectManager[i];
               sprintf_s(debugBuffer, "(%s) pos: (x%f, y%f)\n", _objectManager.getObjectName(debugObject).c_str(), debugObject->getPosition().x, debugObject->getPosition().y);
               OutputDebugString(debugBuffer);
            }
            timer = 0.0f;
         }
#endif
         //if (Engine2D::GetInput())

      }

      void onExit(void)
      {
         // Figure out pausing...? (Wrapping this in a 'paused' bool before pushing the PauseState?)
         _player->shutdown();

         _objectManager.clearOperators();
         _objectManager.removeObject("Turd");

         delete _player->getGameObject();
         delete _player;

         delete _updateBackground.getBackground();

         delete _camera;

         GameState::onExit();
      }
   };

   class GameOverState : public GameState
   {
      Image *_gameOver = NULL;

   public:
      void onEnter(void) {
         GameState::onEnter();

         _gameOver = new Image("./data/images/game_over.png");
         _gameOver->center();

         _renderList->push_back(_gameOver);
      }

      /*void onExecute(float time) {
         
      }*/

      void onExit(void) {
         _renderList->pop_back();
         delete _gameOver;

         GameState::onExit();
      }
   };

   void GameOver(void) {
      this->push(new GameOverState);
   }

   friend PlayState;

public:
   void Begin(void)
   {
      this->push(new PlayState);
   }

   void End(void)
   {
      while (!this->empty()) {
         void* gameState = this->top(); this->pop(); delete gameState;
      }
   }
}game;

#define GLOBAL_WIDTH (640 / 2) /*320*/
#define GLOBAL_HEIGHT 480

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   Window rndrWind = Window(GLOBAL_WIDTH, GLOBAL_HEIGHT, "Flap a Turd", "• • • •");
   rndrWind.Initialize(hInstance, lpCmdLine);

   DirectInput* pInput = (DirectInput*)Input::CreateDirectInputInterface(rndrWind.GetHWND(), hInstance);
   RendererDX* pRenderer = (RendererDX*)Renderer::CreateDXRenderer(rndrWind.GetHWND(), GLOBAL_WIDTH, GLOBAL_HEIGHT, false, false);

   Engine2D* engine = Engine2D::getInstance();
   engine->SetInputInterface(pInput);
   engine->SetRenderer(pRenderer);
   engine->SetGame(&game);
   engine->Initialize();

   while (!rndrWind.HasQuit() && !engine->HasQuit())
   {
      rndrWind.Update();
      engine->Update();
   }

   engine->Shutdown();

   Input::DestroyInputInterface(pInput);
   Renderer::DestroyRenderer(pRenderer);

   rndrWind.Shutdown();
}
// Stan Taveras 