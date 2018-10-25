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

#include "SDSParser.h"

#include "FollowObjectOperator.h"
#include "MaxVelocityOperator.h"
#include "ApplyVelocityOperator.h"
#include "UpdateRenderableOperator.h"
#include "UpdateBackgroundOperator.h"

#include "AnimationLoader.h"

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

         _background = new Image("./data/images/bg.png");
         _background->center();

         _renderList->push_back(_background);

         _updateBackground.useRenderList(_renderList);
         _updateBackground.setBackground(_background);
         _updateBackground.setMode(Background::Mode_Mirror);

         std::map<std::string, Animation*> exampleAnimation = AnimationLoader::loadAnimationsFromXML("./data/example.ani");

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

         _camera = new Camera;
         _objectManager.addObject("Camera", _camera);
         _updateBackground.setCamera(_camera);

         _attachCamera.setSource(_camera);
         _attachCamera.follow(_objectManager.getGameObject("Turd"), true, false);
         
         Engine2D::GetRenderer()->SetCamera(_camera);
      }

      //void onExecute(float time)
      //{
      //   GameState::onExecute(time);
      //}

      void onExit(void)
      {
         // Figure out pausing...? (Wrapping this in a 'paused' bool before pushing the PauseState?)
         delete _camera;

         _player->shutdown();

         _objectManager.clearOperators();
         _objectManager.removeObject("Turd");

         delete _player->getGameObject();
         delete _player;

         _renderList->remove(_background);

         delete _background;

         GameState::onExit();
      }
   };

   friend PlayState;

public:
   void Begin(void)
   {
      this->push(new PlayState);
   }

   void End(void)
   {
      void* playState = this->top(); this->pop(); delete playState;
   }
}game;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   Window rndrWind = Window(320, 460, "Flap a Turd", "· · · ·");
   rndrWind.Initialize(hInstance, lpCmdLine);

   DirectInput* pInput = (DirectInput*)Input::CreateDirectInputInterface(rndrWind.GetHWND(), hInstance);
   RendererDX* pRenderer = (RendererDX*)Renderer::CreateDXRenderer(rndrWind.GetHWND(), 320, 480, false, false);

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