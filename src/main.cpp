// main.cpp
#include "Engine2D.h"
#include "Input.h"
#include "InputEvent.h"
#include "Game.h"
#include "GameState.h"
#include "GameObject.h"
#include "Renderer.h"
#include "Window.h"

#include "SDSParser.h"
class FlappyTurd : public Game
{
   IRenderer::RenderList* _sprites;

   class PlayState : public GameState
   {
      FlappyTurd* _game;
      Sprite* _bg;

      class Turd : public Player, GameObject
      {
         friend PlayState;

         class TurdState : public GameObjectState
         {
         public:
            void OnEnter()
            {
            }

            void OnExit()
            {

            }
         };

      public:
         Turd(void)
         {
            GameObjectState* Falling = this->AddState("Falling");
            Falling->setDirection(vector2(1,-1));
            Falling->setForce(3);

            GameObjectState* Rising = this->AddState("Rising");
            Rising->setDirection(vector2(1,1));
            Rising->setForce(3);

            this->RegisterTransition("Falling","BUTTON_PRESSED","Rising");
            this->RegisterTransition("Rising","TIME_PASSED","Falling");
         }
      }_Player;

   public:
      void OnKeyPressed(const Event& e)
      {
         InputEvent* inputEvent = (InputEvent*)&e;
         std::string stateMachineInput = inputEvent->GetButtonID() + "_PRESSED";
         _Player.GetGameObject()->SendInput(stateMachineInput.c_str(),e.GetSender());
      }

      void OnKeyReleased(const Event& e)
      {
         InputEvent* inputEvent = (InputEvent*)&e;
         std::string stateMachineInput = inputEvent->GetButtonID() + "_RELEASED";
         _Player.GetGameObject()->SendInput(stateMachineInput.c_str(),e.GetSender());
      }

	  // TODO: Move the above two functions to Turd

      void OnEnter(void)
      {
         GameState::OnEnter();

         Engine2D* engine = Engine2D::GetInstance();
         engine->GetEventSystem()->RegisterCallback<PlayState>("EVT_KEYPRESSED", this, &PlayState::OnKeyPressed);
         engine->GetEventSystem()->RegisterCallback<PlayState>("EVT_KEYRELEASED", this, &PlayState::OnKeyReleased);

         _InputManager.Initialize(engine->GetEventSystem(), engine->GetInput());

         _game = (FlappyTurd*)engine->GetGame();

         _bg = this->_Sprites.Create(Sprite("./data/images/bg.png"));

         _game->PushSprite(_bg);

         _ObjectManager.AddObject("Player",&_Player);
         _Player.SetGameObject(&_Player);
         _Player.SetGamePad(_InputManager.CreateGamePad());
         _Player.GetGamePad()->AddButton(VirtualButton("BUTTON",KBK_SPACE));

         GameObject::GameObjectState* Falling = (GameObject::GameObjectState*)_Player.GetState("Falling");
         Falling->SetRenderable(_Sprites.Create(Sprite("./data/images/turd0.png",0xFFFF00FF)));

         GameObject::GameObjectState* Rising = (GameObject::GameObjectState*)_Player.GetState("Rising");
         Rising->SetRenderable(_Sprites.Create(Sprite("./data/images/turd1.png",0xFFFF00FF)));
         Rising->GetRenderable()->SetVisibility(false);

		 // TODO: Where are these events registered?

         _game->PushSprite(Falling->GetRenderable());
         _game->PushSprite(Rising->GetRenderable());

         //SDSParser *scriptParser = 
         // camera follows turd + bg
      }

      void OnExecute(float time)
      {
         GameState::OnExecute(time);
      }

      void OnExit(void)
      {
         _game->PopSprite();
         _ObjectManager.RemoveObject("Player");

         this->_Sprites.Destroy(_bg);
         GameState::OnExit();
      }
   }_PlayState;

public:
   void PushSprite(Renderable* bg)
   {
      _sprites->push_back(bg);
   }

   void PopSprite()
   {
      _sprites->pop_back();
   }

   void Begin(void)
   {
      _sprites = Renderer::Get()->CreateRenderList();

      this->push(&_PlayState);
   }
   
   void End(void)
   {
      this->pop();

      Renderer::Get()->DestroyRenderList(_sprites);
   }
}game;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
   Window rndrWind = Window(320, 460, "Flap a Turd", "· · · ·");
   rndrWind.Initialize(hInstance, lpCmdLine);

   DirectInput* pInput = (DirectInput*)Input::CreateDirectInputInterface(rndrWind.GetHWND(), hInstance);
   RendererDX* pRenderer = (RendererDX*)Renderer::CreateDXRenderer(rndrWind.GetHWND(), 320, 480, false, false);

   Engine2D* engine = Engine2D::GetInstance();
   engine->SetInputInterface(pInput);
   engine->SetRenderer(pRenderer);
   engine->SetGame(&game);
   engine->Initialize();

   while(!rndrWind.HasQuit() && !engine->HasQuit())
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