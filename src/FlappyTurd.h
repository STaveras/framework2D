#pragma once

#define GLOBAL_WIDTH (640 / 2) /*320*/
#define GLOBAL_HEIGHT 480

#define FALL_FORCE 100.0f
#define FLAP_MULTIPLIER 3.33f

class FlappyTurd : public Game
{
   class PlayState : public GameState
   {
      // (Probably should just go in GameState...?)
      Image* _background = NULL; // Lights; the set

      // Game rules
      FollowObject _attachCamera;
      // MaxVelocityOperator _maxVelocity;
      ApplyVelocityOperator _applyVelocity;
      UpdateBackgroundOperator _updateBackground;
      UpdateRenderableOperator _updateRenderable;
      // TODO: UpdateCollidable _updateCollidable;

      class Turd : public GameObject
      {
         // Currently unused
         // class TurdState : public GameObjectState
         //{
         // public:
         // void onEnter()
         // {
         //	 //GameObjectState::onEnter();
         // }

         // void onExit()
         // {

         // }
         //};

      public:
         Turd(void) : GameObject(GAME_OBJ_OBJECT)
         {
            // NOTE: All of this should ideally be in a script
            GameObjectState* falling = this->addState("Falling");
            // falling->setDirection(vector2(0.1019108280254777f, 0.1464968152866242f)); // normalized it myself ;)
            falling->setDirection(vector2(0.1f, 1.0f));
            falling->setForce(FALL_FORCE);
            falling->setRenderable(new Sprite("./data/images/turd0.png", 0xFFFF00FF));
            ((Image*)falling->getRenderable())->center();

            GameObjectState* rising = this->addState("Rising");
            // rising->setDirection(vector2(0.1f, -1.0f));
            rising->setExecuteTime(0.27);
            rising->setDirection(vector2(0.1f, -1.0f));
            rising->setForce(FALL_FORCE * FLAP_MULTIPLIER);
            rising->setRenderable(new Sprite("./data/images/turd1.png", 0xFFFF00FF));
            ((Image*)rising->getRenderable())->center();

            RegisterTransition("Falling", "BUTTON_PRESSED", "Rising");
            RegisterTransition("Rising", "BUTTON_PRESSED", "Rising");   // lets you chain together flaps
            RegisterTransition("Rising", "BUTTON_RELEASED", "Falling"); // stop rising when you let go of the button
         }

         ~Turd(void)
         {
            for (unsigned int i = 0; i < _states.Size(); i++)
               delete ((GameObjectState*)_states.At(i))->getRenderable();
         }
      }*_turd;

   public:
      void onEnter(void)
      {
         GameState::onEnter();

         //_background = new Image("./data/images/bg.png");

         _objectManager.addObject("Camera", _camera);

         _updateBackground.useRenderList(_renderList);
         _updateBackground.setCamera(_camera);
         _updateBackground.setBackground(_background);
         _updateBackground.setMode(Background::Mode::Mirror);

         //Animations::addToRenderList(Animations::fromXML("./data/objects/turd/turd.ani"), _renderList);

         _objectManager.addObject("Turd", [&] {_turd = new Turd; return _turd; }());
         _objectManager.pushOperator(&_updateBackground);

         // -- The below are to simulate ''gamey'' physics instead of having *real* physics
         _objectManager.pushOperator(&_applyVelocity);
         //_objectManager.pushOperator(&_maxVelocity);
         // --                               ^ ^ ^ ^                                  -- //

         _objectManager.pushOperator(&_updateRenderable);
         _objectManager.pushOperator(&_attachCamera);

         _player->setup();
         _player->setGamePad(_inputManager.CreateGamePad());
         _player->getGamePad()->addButton(VirtualButton("BUTTON", KBK_SPACE));
         _player->setGameObject(_objectManager.getGameObject("Turd"));

         _objectManager.addObject("Camera", _camera);
         _updateBackground.setCamera(_camera);
         //_camera->SetZoom(0.5);

         _attachCamera.setSource(_camera);
         _attachCamera.follow(_objectManager.getGameObject("Turd"), true, false);

         Engine2D::GetRenderer()->SetCamera(_camera);
      }
      void onExecute(float time)
      {
         if (_objectManager.getGameObject("Turd")->getPosition().y < (_camera->GetScreenHeight() / 2)) {
            GameState::onExecute(time);
         }
         else {// Push on 'GameOverState'
            ((FlappyTurd*)Engine2D::GetGame())->GameOver();
         }

#if _DEBUG
			static float timer = 0.0f;
			timer += time;

			if (timer >= 1.0f) {

				if (Debug::dbgObjects) {

					char debugBuffer[255];

               OutputDebugString("\nObjects:\n-------\n");

					for (unsigned int i = 0; i < _objectManager.numObjects(); i++) {
                  GameObject* debugObject = _objectManager[i];
						sprintf_s(debugBuffer, "(%s) pos: (x%f, y%f)\n", _objectManager.getObjectName(debugObject).c_str(), debugObject->getPosition().x, debugObject->getPosition().y);
						OutputDebugString(debugBuffer);
					}
				}

            if (_camera) {
               if (!_camera->OnScreen(_player->getGameObject())) {
                  //Engine2D::getEventSystem()->sendEvent("EVT_GAME_OVER");
                  OutputDebugString("GameObject off screen\n\n");
               }
            }

				timer = 0.0f;
			}
#endif

         if (Engine2D::GetInput()->GetKeyboard()->KeyPressed(KBK_ESCAPE)) {
            Engine2D::Quit();
         }
      }

      void onExit(void)
      {
         // I feel like players shouldn't "shutdown"
         _player->shutdown();

         _objectManager.clearOperators();
         _objectManager.removeObject("Turd");
         _updateBackground.setBackground(NULL);

         // The below isn't really that safe, but since we know we're allocating our animations here manually... 
         IRenderer::RenderList::iterator i = _renderList->begin();
         for (; i != _renderList->end(); i++) {
            if ((*i)->getRenderableType() == RENDERABLE_TYPE_ANIMATION)
               Animations::destroyAnimation((Animation*)(*i));
         }

         delete _player->getGameObject();
         delete _updateBackground.getBackground();

         GameState::onExit();
      }
   }*_playState;

   class GameOverState : public GameState
   {
      Image* _gameOver = NULL;

   public:
      void onEnter(void) {
         GameState::onEnter();

         _gameOver = new Image("./data/images/game_over.png");
         _gameOver->center();
         _gameOver->setPosition(Engine2D::GetRenderer()->GetCamera()->getPosition());

         _renderList->push_back(_gameOver);
      }

      void onExecute(float time) {
         if (Engine2D::GetInput()->GetKeyboard()->KeyPressed(KBK_SPACE)) {
            ((FlappyTurd*)Engine2D::GetGame())->Reset();
         }
      }

      void onExit(void) {
         _renderList->pop_back();
         delete _gameOver;

         GameState::onExit();
      }
   };

   void GameOver(void) {
      this->push(new GameOverState);
   }

   void StartGame(void) {
      if (!_playState) {
         this->push(new PlayState);
         _playState = (PlayState*)this->top();
      }
   }

   void EndGame(void) {
      if (_playState) {
         while (!this->empty()) {
            this->pop();
         }
         delete _playState;
         _playState = NULL;
      }
   }

   friend PlayState;

public:

   void Reset(void) {
      EndGame();
      StartGame();
   }

   void Begin(void)
   {
      StartGame();
   }

   void End(void)
   {
      Engine2D::getEventSystem()->FlushEvents();

      IProgramState* playState = this->top();
      this->clear();
      delete playState;

      //while (!this->empty()) {
      //   void* gameState = this->top(); this->pop(); delete gameState;
      //}
   }
} game;