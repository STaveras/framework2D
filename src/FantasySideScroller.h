// FantasySideScroller
// An example game using this framework
////////////////////////////////////////

#pragma once

#define BASE_DIRECTORY "./fantasySideScroller/"

#define GAME_RES_X 336
#define GAME_RES_Y 192

#include "Game.h"
#include "GameObject.h"
#include "GameState.h"

#include "AnimationUtils.h"

#include "Square.h"

#define FALL_FORCE 100.0f
#define FLAP_MULTIPLIER 3.33f

// Game should hold all the managers
class FantasySideScroller : public Game
{
	// States should just queue up operators, potentially stacking up other states
	class PlayState : public GameState
	{
		// (Probably should just go in GameState...?)
		Image* _background = NULL; // Lights; the set

		// Game rules
		//FollowObject _attachCamera;
		//ApplyVelocityOperator _applyVelocity;
		//UpdateBackgroundOperator _updateBackground;

		class Character : public GameObject
		{
		public:
			Character(void) : GameObject(GAME_OBJ_OBJECT)
			{
				GameObjectState* idle = this->addState("Idle");

				Animation* idleAnimation = _animationManager.Create();

				vector2 idleFrameDimensions{ 64, 80 };

				Texture* idleSheet = Engine2D::getRenderer()->CreateTexture(BASE_DIRECTORY"Character/Idle/Idle-Sheet.png");

				Animations::createFramesForAnimation(idleAnimation, idleSheet, idleFrameDimensions, _spriteManager);

				idleAnimation->setMode(Animation::Mode::eLoop);
				idleAnimation->setName(idle->getName());
				idleAnimation->setFrameRate(30);
				idleAnimation->center();

				idle->setPreserveMirroring(true);
				idle->setRenderable(idleAnimation);

				//static Square idleHitBox({ 0,0 }, (idleFrameDimensions * 0.8f));
				//for (unsigned int i = 0; i < idleAnimation->getFrameCount(); i++) {
				//   (*idleAnimation)[i]->GetSprite()->SetCollisionInfo(&idleHitBox);
				//}

				/////////////////////////////////////////
				GameObjectState* rising = this->addState("Rising");

				rising->setDirection(vector2(0.0f, -1.0f));
				rising->setForce(FALL_FORCE * FLAP_MULTIPLIER);
				rising->setPreserveMirroring(true);
				rising->setExecuteTime(0.33);

				Animation* risingAnimation = _animationManager.Create();

				vector2 risingDimensions{ 64, 64 };

				Texture* risingSheet = Engine2D::getRenderer()->CreateTexture(BASE_DIRECTORY"Character/Jump-Start/Jump-Start-Sheet.png");

				Animations::createFramesForAnimation(risingAnimation, risingSheet, risingDimensions, _spriteManager);

				risingAnimation->setName(rising->getName());
				risingAnimation->setOffset({ 0.0, -8.0 });
				risingAnimation->setFrameRate(30);
				risingAnimation->center();

				rising->setRenderable(risingAnimation);

				/////////////////////////////////////////

				GameObjectState* falling = this->addState("Falling");
				falling->setDirection(vector2(0.0f, 1.0f));
				falling->setPreserveMirroring(true);
				//falling->setPreserveScale(true);
				falling->setForce(FALL_FORCE);

				vector2 fallingDimensions{ 64, 64 };

				Texture* fallingSheet = Engine2D::getRenderer()->CreateTexture(BASE_DIRECTORY"Character/Jump-End/Jump-End-Sheet.png");

				Animation* fallingAnimation = _animationManager.Create();
				fallingAnimation->setName(falling->getName());
				fallingAnimation->setOffset({ 0.0, -8.0 });
				falling->setRenderable(fallingAnimation);

				Animations::createFramesForAnimation(fallingAnimation, fallingSheet, fallingDimensions, _spriteManager);
				fallingAnimation->setFrameRate(30);
				fallingAnimation->center();

				/////////////////////////////////////////

				GameObjectState* runningLeft = this->addState("RunningLeft");
				GameObjectState* runningRight = this->addState("RunningRight");

				vector2 runningDimensions{ 80.0, 80.0 };

				runningLeft->setDirection({ -1.0, 0.0 });
				runningRight->setDirection({ 1.0, 0.0 });

				Texture* runningSheet = Engine2D::getRenderer()->CreateTexture(BASE_DIRECTORY"Character/Run/Run-Sheet.png");

				Animation* runningLeftAnimation = _animationManager.Create();
				Animation* runningRightAnimation = _animationManager.Create();

				runningLeftAnimation->setName(runningLeft->getName());
				runningRightAnimation->setName(runningRight->getName());

				runningLeft->setRenderable(runningLeftAnimation);
				runningRight->setRenderable(runningRightAnimation);

				Animations::createFramesForAnimation(runningLeftAnimation, runningSheet, runningDimensions, _spriteManager);
				Animations::createFramesForAnimation(runningRightAnimation, runningSheet, runningDimensions, _spriteManager);

				runningLeftAnimation->mirror(true, false);
				runningLeftAnimation->setMode(Animation::Mode::eLoop);
				runningLeftAnimation->setFrameRate(60);
				runningLeftAnimation->setSpeed(1.1f);
				runningLeftAnimation->center();

				runningRightAnimation->setMode(Animation::Mode::eLoop);
				runningRightAnimation->setFrameRate(60);
				runningRightAnimation->setSpeed(1.1f);
				runningRightAnimation->center();

				registerTransition("Idle", "JUMP_PRESSED", "Rising");
				registerTransition("Idle", "LEFT_PRESSED", "RunningLeft");
				registerTransition("Idle", "RIGHT_PRESSED", "RunningRight");
				registerTransition("Rising", "JUMP_RELEASED", "Falling"); // stop rising when you let go of the button
				registerTransition("Rising", EVT_STATE_END, "Falling"); // fall when time reached
				registerTransition("RisingRight", EVT_STATE_END, "Falling"); // fall when time reached
				registerTransition("Falling", "GROUND_COLLISION", "Idle");
				registerTransition("Falling", EVT_STATE_END, "Idle");
				registerTransition("RunningLeft", "LEFT_RELEASED", "Idle");
				registerTransition("RunningLeft", "RIGHT_PRESSED", "RunningRight");
				registerTransition("RunningRight", "RIGHT_RELEASED", "Idle");
				registerTransition("RunningRight", "LEFT_PRESSED", "RunningLeft");
				registerTransition("RunningRight", "JUMP_PRESSED", "RisingRight");
				registerTransition("RunningLeft", "JUMP_PRESSED", "Rising");
				registerTransition("RunningRight", "JUMP_PRESSED", "Rising");

				//collisionEventHandler = [=](const CollisionEvent* e) {
				//   
				//   if (((GameState*)Engine2D::getGame()->top())->getObjectManager()->getObjectName(e->involvedObject) == "GroundTile")
				//      this->sendInput("GROUND_COLLISION");

				//};
			}

			void update(float fTime) {

				GameObject::update(fTime);

				//if (!strcmp(this->GetCurrentState()->getName(), "Idle")) {
				//   Engine2D::GetInput()->GetKeyboard()->KeyDown()

				//}
			}

			~Character(void) {
				//for (unsigned int i = 0; i < _states.Size(); i++)
				//   delete ((GameObjectState*)_states.At(i))->getRenderable();
			}
		}*playableCharacter = NULL;

	public:
		void onEnter(State* prev)
		{
			GameState::onEnter();

			_background = new Image(BASE_DIRECTORY"Background/Background.png");
			_background->center();

			_renderList->push_back(_background);

			playableCharacter = new Character;

			_objectManager.addObject("Hero", playableCharacter);
			_objectManager.addObject("Camera", _camera);

			_player->start();
			_player->setController(_inputManager.CreateController());
			_player->getController()->addAction(Action("JUMP", KBK_SPACE));
			_player->getController()->addAction(Action("LEFT", KBK_LEFT));
			_player->getController()->addAction(Action("RIGHT", KBK_RIGHT));
			_player->setGameObject(_objectManager.getGameObject("Hero"));

			Engine2D::getRenderer()->SetCamera(_camera);
		}

		bool onExecute(float time)
		{
			GameState::onExecute(time);

			// TODO: Handle enemy spawning, game rules, etc.

			GameObject* playerObject = _objectManager.getGameObject("Hero");

			if (playerObject->getPosition().y + 80 >= GAME_RES_Y) {
				_objectManager.getGameObject("Hero")->sendInput("GROUND_COLLISION");
			}

			if (Engine2D::getInput()->GetKeyboard()->KeyPressed(KBK_ESCAPE)) {

				// TODO: Bring up a menu (i.e. push a 'MenuState')

				Engine2D::quit();
			}

			return true;
		}

		void onExit(State* next)
		{
			// I feel like players shouldn't "shutdown"
			_player->shutdown();

			_objectManager.removeObject("Camera");
			_objectManager.removeObject("Hero");

			if (playableCharacter) {
				delete playableCharacter;
			}

			if (_background) {
				delete _background;
			}

			GameState::onExit();
		}
	}*_playState = NULL;

public:

	void Begin(void)
	{
		// load options and/or configurations

		Renderer::window->setWindowTitle("\'FantasySideScroller\'");
		Renderer::window->setWidth(GAME_RES_X);
		Renderer::window->setHeight(GAME_RES_Y);

		Renderer::Get()->setWidth(GAME_RES_X);
		Renderer::Get()->setHeight(GAME_RES_Y);

		Renderer::Get()->shutdown();
		Renderer::window->resize();
		Renderer::Get()->initialize();

		if (!_playState)
		{
			_playState = new PlayState();

			// Do some preloading here

			this->push(_playState);
		}
	}

	void End(void)
	{
		if (_playState) {
			delete _playState;
		}
	}

} game;