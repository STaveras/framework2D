// FantasySideScroller
// An example game using this framework
////////////////////////////////////////

#pragma once

#define BASE_DIRECTORY "./fantasySideScroller/"

#define WINDOW_SIZE_MULTIPLIER 2
#define GAME_RES_X 336
#define GAME_RES_Y 192

#include "Game.h"
#include "GameObject.h"
#include "GameState.h"

#include "Tile.h"
#include "TileMap.h"

#include "AnimationUtils.h"
#include "FollowObjectOperator.h"

#include "Square.h"

#define MOVE_UNITS 150.0f
#define JUMP_MULTIPLIER 2.67f

// Game should hold all the managers
class FantasySideScroller : public Game
{
	// States should just queue up operators, potentially stacking up other states
	class PlayState : public GameState
	{
		// (Probably should just go in GameState...?)
		Image* _background = NULL; 

		TileSet* _tileSet;
		TileMap* _tileMap;

		AttachObjectsOperator _cameraPlayerAttach;
		//AttachObjectsOperator _backgroundCameraAttach;

		class Character : public GameObject
		{
		public:
			Character(void) : GameObject(GAME_OBJ_OBJECT)
			{
				GameObjectState* idle = this->addState("Idle");

				Animation* idleAnimation = _animationManager.create();

				vector2 idleFrameDimensions{ 64, 80 };

				Texture* idleSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Idle/Idle-Sheet.png");

				Animations::createFramesForAnimation(idleAnimation, idleSheet, idleFrameDimensions, _spriteManager);

				idleAnimation->setName(idle->getName());
				idleAnimation->setMode(Animation::Mode::eOscillate);
				idleAnimation->setFrameRate(30);
				idleAnimation->center();

				idle->setPreserveScaling(true);
				idle->setRenderable(idleAnimation);

				static Square idleHitBox({ 0,0 }, (idleFrameDimensions * 0.8f));
				for (unsigned int i = 0; i < idleAnimation->getFrameCount(); i++) {
				   (*idleAnimation)[i]->setCollidable(&idleHitBox);
				}

				/////////////////////////////////////////
				GameObjectState* rising = this->addState("Rising");
				rising->setPreserveScaling(true);
				rising->setExecuteTime(0.1);
				//rising->setDirection(vector2(0.0f, -1.0f));
				//rising->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.1));

				Animation* risingAnimation = _animationManager.create();

				vector2 risingDimensions{ 64, 64 };

				Texture* risingSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Jump-Start/Jump-Start-Sheet.png");

				Animations::createFramesForAnimation(risingAnimation, risingSheet, risingDimensions, _spriteManager);

				risingAnimation->setName(rising->getName());
				risingAnimation->setOffset({ 0.0, -8.0 });
				risingAnimation->setFrameRate(30);
				risingAnimation->center();

				rising->setRenderable(risingAnimation);

				/////////////////////////////////////////

				GameObjectState* jump = this->addState("Jump");
				jump->setPreserveScaling(true);
				jump->setExecuteTime(0.2);
				jump->setDirection(vector2(0.0f, -1.0f));
				jump->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.67));

				Animation* jumpAnimation = _animationManager.create();

				vector2 jumpDimensions{ 64, 64 };

				Texture* jumpSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Jumlp-All/Jump-All-Sheet.png");

				Animations::createFramesForAnimation(jumpAnimation, jumpSheet, jumpDimensions, _spriteManager, 4, 8);

				jumpAnimation->setName(jump->getName());
				jumpAnimation->setMode(Animation::Mode::eLoop);
				jumpAnimation->setOffset({ 0.0, -8.0 });
				jumpAnimation->setFrameRate(60);
				jumpAnimation->center();

				static Square jumpHitBox({ 0,0 }, (jumpDimensions * 0.8f));
				for (unsigned int i = 0; i < jumpAnimation->getFrameCount(); i++) {
					(*jumpAnimation)[i]->setCollidable(&jumpHitBox);
				}

				jump->setRenderable(jumpAnimation);

				/////////////////////////////////////////

				GameObjectState* falling = this->addState("Falling");
				falling->setPreserveScaling(true);
				falling->setDirection(vector2(0.0f, 1.0f));
				falling->setForce((MOVE_UNITS * JUMP_MULTIPLIER) * 0.5);

				falling->setRenderable(jumpAnimation);

				/////////////////////////////////////////

				GameObjectState* landing = this->addState("Landing");
				landing->setDirection(vector2(0.0f, 1.0f));
				landing->setPreserveScaling(true);

				vector2 landingDimensions{ 64, 64 };

				Texture* landingSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Jump-End/Jump-End-Sheet.png");

				Animation* landingAnimation = _animationManager.create();
				landingAnimation->setName(landing->getName());
				landingAnimation->setOffset({ 0.0, -8.0 });
				landingAnimation->setSpeed(2.7f);
				landing->setRenderable(landingAnimation);

				Animations::createFramesForAnimation(landingAnimation, landingSheet, landingDimensions, _spriteManager);
				landingAnimation->setFrameRate(30);
				landingAnimation->center();

				/////////////////////////////////////////

				GameObjectState* runningLeft = this->addState("RunningLeft");
				GameObjectState* runningRight = this->addState("RunningRight");

				vector2 runningDimensions{ 80.0, 80.0 };

				runningLeft->setDirection({ -1.0, 0.0 });
				runningRight->setDirection({ 1.0, 0.0 });

				runningLeft->setForce(100);
				runningRight->setForce(100);

				Texture* runningSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Run/Run-Sheet.png");

				Animation* runningLeftAnimation = _animationManager.create();
				Animation* runningRightAnimation = _animationManager.create();

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

				/////////////////////////////////////////

				GameObjectState* attack01 = this->addState("Attack01");
				GameObjectState* attack02 = this->addState("Attack02");

				attack01->setPreserveScaling(true);
				attack02->setPreserveScaling(true);

				vector2 attackDimensions{ 96.0, 80.0 };

				Texture* attackSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Attack-01/Attack-01-Sheet.png");

				Animation* attack01Animation = _animationManager.create();
				Animations::createFramesForAnimation(attack01Animation, attackSheet, attackDimensions, _spriteManager, 0, 5);
				attack01Animation->setName(attack01->getName());
				attack01Animation->setFrameRate(60);
				attack01Animation->center();
				attack01->setRenderable(attack01Animation);

				Animation* attack02Animation = _animationManager.create();
				Animations::createFramesForAnimation(attack02Animation, attackSheet, attackDimensions, _spriteManager, 5, 3);
				attack02Animation->setName(attack02->getName());
				attack02Animation->setFrameRate(30);
				attack02Animation->center();
				attack02->setRenderable(attack02Animation);

				/////////////////////////////////////////

				GameObjectState* dead = this->addState("Dead");
				dead->setPreserveScaling(true);

				vector2 deadDimensions{ 80, 64 };

				Texture* deadSheet = Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Character/Dead/Dead-Sheet.png");

				Animation* deadAnimation = _animationManager.create();

				Animations::createFramesForAnimation(deadAnimation, deadSheet, deadDimensions, _spriteManager);

				deadAnimation->setName(dead->getName());
				deadAnimation->setOffset({ 8.0, 8.0 });
				deadAnimation->setFrameRate(30);
				deadAnimation->center();

				dead->setRenderable(deadAnimation);

				// If running, and you press jump, return to running instead of idle
				// running, press jump, jumping, if previous state running, on end, return to running
				/////////////////////////////////////////

				registerTransition("Dead", "DEATH", "Idle");

				registerTransition("Idle", "JUMP_PRESSED", "Rising");
				registerTransition("Idle", "LEFT_DOWN", "RunningLeft");
				registerTransition("Idle", "RIGHT_DOWN", "RunningRight");
				registerTransition("Idle", "ATTACK_PRESSED", "Attack01");
				registerTransition("Idle", "DEATH", "Dead");

				registerTransition("Rising", EVT_STATE_END, "Jump");
				registerTransition("Rising", "JUMP_UP", "Falling"); // stop rising when you let go of the button
				registerTransition("Rising", "JUMP_RELEASED", "Falling"); // stop rising when you let go of the button

				registerTransition("Jump", "JUMP_UP", "Falling"); // stop rising when you let go of the button
				registerTransition("Jump", "JUMP_RELEASED", "Falling"); // stop rising when you let go of the button
				registerTransition("Jump", EVT_STATE_END, "Falling"); // stop rising when you let go of the button

				registerTransition("Falling", "GROUND_COLLISION", "Landing");
				registerTransition("Landing", EVT_STATE_END, "Idle");

				registerTransition("RunningLeft", "LEFT_RELEASED", "Idle");
				registerTransition("RunningLeft", "RIGHT_PRESSED", "RunningRight");
				registerTransition("RunningLeft", "JUMP_PRESSED", "Rising");
				registerTransition("RunningLeft", "ATTACK_PRESSED", "Attack01");
				 
				registerTransition("RunningRight", "RIGHT_RELEASED", "Idle");
				registerTransition("RunningRight", "LEFT_PRESSED", "RunningLeft");
				registerTransition("RunningRight", "JUMP_PRESSED", "Rising");
				registerTransition("RunningRight", "ATTACK_PRESSED", "Attack01");

				registerTransition("Attack01", EVT_STATE_END, "Idle");
				registerTransition("Attack01", "ATTACK_PRESSED", "Attack02");
				registerTransition("Attack02", EVT_STATE_END, "Idle");

				this->setState(falling);
				this->setMass(100);
				//this->setBuffered(true); // TODO: Buffered inputs don't seem to be working atm...

				collisionEventHandler = [=](const CollisionEvent* e) {

					GameObject* otherObject = e->involvedObject;
					if (otherObject->getPosition().y > this->getPosition().y) {
						this->sendInput("GROUND_COLLISION");
					}
				};
			}

			void update(float time) {
				
				GameObjectState* state = this->getState();

				if (state) {

					if (!strcmp(state->getName(), "Jump") || !strcmp(state->getName(), "Falling")) {

						// We should instead lookup the bound action or instead override "sendEvent" capture the sent events?
						if (Engine2D::getInput()->getKeyboard()->KeyDown(KBK_LEFT)) {
							this->setPosition(this->getPosition().x - MOVE_UNITS * time, this->getPosition().y);
							this->getRenderable()->setScale(-abs(this->getRenderable()->getScale().x), this->getRenderable()->getScale().y);
						}

						if (Engine2D::getInput()->getKeyboard()->KeyDown(KBK_RIGHT)) {
							this->setPosition(this->getPosition().x + MOVE_UNITS * time, this->getPosition().y);
							this->getRenderable()->setScale(abs(this->getRenderable()->getScale().x), this->getRenderable()->getScale().y);
						}
					}
					else if (Engine2D::getInput()->getKeyboard()->KeyPressed(KBK_F)) {
						this->sendInput("DEATH");
					}

					//if (this->getPosition().y > 72) {
					//	this->sendInput("GROUND_COLLISION");
					//}
				}

				GameObject::update(time);
			}

			~Character(void) {

			}
		}*playableCharacter = NULL;

	public:
		void onEnter(State* prev)
		{
			GameState::onEnter();

			_background = new Image(BASE_DIRECTORY"Background/Background.png");
			_background->center();

			_renderList->push_back(_background);

			_tileSet = new TileSet(Engine2D::getRenderer()->createTexture(BASE_DIRECTORY"Assets/Tiles.png"), 16);

			_tileMap = TileMap::loadFromCSVFile(BASE_DIRECTORY"testMap_2.csv", _tileSet);

			for (size_t i = 0; i < _tileMap->getTiles().size(); i++) {
				char buffer[32]{ 0 };
				sprintf_s(buffer, 32, "t%i", i);
				_objectManager.addObject(buffer, _tileMap->getTiles()[i]);
			}

			_tileMap->setPosition(-120, 0);
			_tileMap->arrangeTiles();

			playableCharacter = new Character;
			playableCharacter->setPosition(0, -120);

			_objectManager.addObject("Hero", playableCharacter);
			_objectManager.addObject("Camera", _camera);

			_player->start();
			_player->setController(_inputManager.createController());
			_player->getController()->addAction(Action("JUMP", KBK_SPACE));
			_player->getController()->addAction(Action("LEFT", KBK_LEFT));
			_player->getController()->addAction(Action("RIGHT", KBK_RIGHT));
			_player->getController()->addAction(Action("ATTACK", KBK_LCONTROL));
			_player->setGameObject(playableCharacter);

			_cameraPlayerAttach.setSource(_camera);
			_cameraPlayerAttach.follow(_objectManager.getGameObject("Hero"), true, true);
			_cameraPlayerAttach.setEnabled(true);

			_objectManager.pushOperator(&_cameraPlayerAttach);

			Engine2D::getRenderer()->setCamera(_camera);
		}

		bool onExecute(float time)
		{
			GameState::onExecute(time);

			// TODO: Handle enemy spawning, game rules, etc.

			if (Engine2D::getInput()->getKeyboard()->KeyPressed(KBK_R)) {
				playableCharacter->setState(playableCharacter->getState("Falling"));
				playableCharacter->setPosition(0, -120);
			}

			if (Engine2D::getInput()->getKeyboard()->KeyPressed(KBK_ESCAPE)) {

				// TODO: Bring up a menu (i.e. push a 'MenuState')

				Engine2D::quit();
			}

			return true;
		}

		void onExit(State* next)
		{
			//GameObject* groundTile = _objectManager.getGameObject("GroundTile");
			//_objectManager.removeObject(groundTile);
			//delete groundTile;

			_player->finish();

			_objectManager.removeObject("Camera");
			_objectManager.removeObject("Hero");

			if (playableCharacter) {
				delete playableCharacter;
			}

			if (_tileSet) {
				delete _tileSet;
			}

			if (_background) {
				delete _background;
			}

			GameState::onExit();
		}
	}*_playState = NULL;

public:

	void begin(void)
	{
		// load options and/or configurations

		Renderer::window->setWindowTitle(BASE_DIRECTORY);
		Renderer::window->setWidth(GAME_RES_X * WINDOW_SIZE_MULTIPLIER);
		Renderer::window->setHeight(GAME_RES_Y * WINDOW_SIZE_MULTIPLIER);

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

	void end(void)
	{
		if (_playState) {
			delete _playState;
		}
	}

} game;