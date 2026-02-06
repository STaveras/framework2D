// Character.cpp

#include "Character.h"

Character::Character(void) : 
	GameObject(GAME_OBJ_OBJECT),
	_tile(NULL) {

	// TODO: Write a loadObjectFromJSON function to load the character from a JSON file, otherwise, call the init functions directly
	//		 Basically, it calls

	_setupCollisionHandler();
		  _initStates();
	_initTransitions();

	this->setBuffered(false);
	this->setState("Falling");
	this->setMass(100);
}

Character::~Character() {}

void Character::_initStates() {
	GameObjectState* idle = this->addState("Idle");

	Animation* idleAnimation = _animationManager.create();

	vector2 idleFrameDimensions{ 64, 80 };

	Texture* idleSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Idle/Idle-Sheet.png").c_str());

	Animations::createFramesForAnimation(idleAnimation, idleSheet, idleFrameDimensions, _spriteManager);

	idleAnimation->setName(idle->getName());
	idleAnimation->setMode(Animation::Mode::eOscillate);
	idleAnimation->setFrameRate(30);
	idleAnimation->center();

	idle->setPreserveScaling(true);
	idle->setRenderable(idleAnimation);

	static Square idleHitBox({ -10, -16 }, 20, 48);
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
	Texture* risingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-Start/Jump-Start-Sheet.png").c_str());

	Animations::createFramesForAnimation(risingAnimation, risingSheet, risingDimensions, _spriteManager);

	risingAnimation->setName(rising->getName());
	risingAnimation->setOffset({ 0.0, -8.0 });
	risingAnimation->setFrameRate(30);
	risingAnimation->center();

	rising->setRenderable(risingAnimation);

	static Square risingHitBox({ -10, -16 }, 20, 38);
	for (unsigned int i = 0; i < risingAnimation->getFrameCount(); i++) {
		(*risingAnimation)[i]->setCollidable(&risingHitBox);
	}

	/////////////////////////////////////////

	GameObjectState* jump = this->addState("Jump");
	jump->setPreserveScaling(true);
	jump->setExecuteTime(0.2);
	jump->setDirection(vector2(0.0f, -1.0f));
	jump->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.67));

	Animation* jumpAnimation = _animationManager.create();

	vector2 jumpDimensions{ 64, 64 };

	Texture* jumpSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jumlp-All/Jump-All-Sheet.png").c_str());

	Animations::createFramesForAnimation(jumpAnimation, jumpSheet, jumpDimensions, _spriteManager, 4, 8);

	jumpAnimation->setName(jump->getName());
	jumpAnimation->setMode(Animation::Mode::eOscillate);
	jumpAnimation->setOffset({ 0.0, -8.0 });
	jumpAnimation->setFrameRate(60);
	jumpAnimation->center();

	jump->setRenderable(jumpAnimation);

	static Square jumpHitBox({ -10, -16 }, 20, 40);
	for (unsigned int i = 0; i < jumpAnimation->getFrameCount(); i++) {
		(*jumpAnimation)[i]->setCollidable(&jumpHitBox);
	}

	//GameObjectState* jumpLeft = this->addState("JumpLeft");
	//GameObjectState* jumpRight = this->addState("JumpRight");

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

	Texture* landingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-End/Jump-End-Sheet.png").c_str());

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

	Texture* runningSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Run/Run-Sheet.png").c_str());

	Animation* runningLeftAnimation = _animationManager.create();
	Animation* runningRightAnimation = _animationManager.create();

	runningLeftAnimation->setName(runningLeft->getName());
	runningRightAnimation->setName(runningRight->getName());

	runningLeft->setRenderable(runningLeftAnimation);
	runningRight->setRenderable(runningRightAnimation);

	Animations::createFramesForAnimation(runningLeftAnimation, runningSheet, runningDimensions, _spriteManager);
	Animations::createFramesForAnimation(runningRightAnimation, runningSheet, runningDimensions, _spriteManager);

	static Square runHitBox({ 19, 17 }, 26, 41);
	for (unsigned int i = 0; i < runningRightAnimation->getFrameCount(); i++) {
		(*runningLeftAnimation)[i]->setCollidable(&runHitBox);
		(*runningRightAnimation)[i]->setCollidable(&runHitBox);
	}

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

	Texture* attackSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Attack-01/Attack-01-Sheet.png").c_str());

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

	Texture* deadSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Dead/Dead-Sheet.png").c_str());

	Animation* deadAnimation = _animationManager.create();

	Animations::createFramesForAnimation(deadAnimation, deadSheet, deadDimensions, _spriteManager);

	deadAnimation->setName(dead->getName());
	deadAnimation->setOffset({ 8.0, 8.0 });
	deadAnimation->setFrameRate(30);
	deadAnimation->center();

	dead->setRenderable(deadAnimation);

	// This isn't really used for much at the moment...
	// But I'd like to eventually store the collision data from each of the frames
	std::vector<Animation*> animations;
	for (auto& animation : _animationManager) {
		if (animation) {
			animations.push_back(animation);
		}
	}
	Animations::toJSON(animations, BasePath("Character/Animations.json").c_str());
	/////////////////////////////////////////////////////////////////////////////
}

void Character::_initTransitions() {

	std::string transitionsFilePath = BasePath("Character/Transitions.json");
	if (FileSystem::FileExists(transitionsFilePath))
	{
		FileStream fileStream = FileSystem::File::Open(transitionsFilePath);
		this->fromJSON(fileStream);
		fileStream.close();
	}
	else
	{
		// If running, and you press jump, return to running instead of idle
		// running, press jump, jumping, if previous state running, on end, return to running
		/////////////////////////////////////////
		registerTransition("Dead", "DEATH", "Idle");

		registerTransition("Idle", "JUMP_PRESSED", "Rising");
		registerTransition("Idle", "LEFT_DOWN", "RunningLeft");
		registerTransition("Idle", "RIGHT_DOWN", "RunningRight");
		registerTransition("Idle", "ATTACK_PRESSED", "Attack01");
		registerTransition("Idle", "IN_AIR", "Falling");
		registerTransition("Idle", "DEATH", "Dead");

		registerTransition("Rising", "JUMP_UP", "Falling"); // stop rising when you let go of the button
		registerTransition("Rising", "JUMP_RELEASED", "Falling");
		registerTransition("Rising", EVT_STATE_END, "Jump");

		registerTransition("Jump", "JUMP_UP", "Falling");
		registerTransition("Jump", "JUMP_RELEASED", "Falling");
		registerTransition("Jump", EVT_STATE_END, "Falling");
		registerTransition("Jump", "DEATH", "Dead");

		registerTransition("Falling", "GROUND_COLLISION", "Landing");
		registerTransition("Falling", "DEATH", "Dead");
		registerTransition("Landing", EVT_STATE_END, "Idle");
		registerTransition("Landing", "DEATH", "Dead");

		registerTransition("RunningLeft", "LEFT_RELEASED", "Idle");
		registerTransition("RunningLeft", "RIGHT_PRESSED", "RunningRight");
		registerTransition("RunningLeft", "JUMP_PRESSED", "Rising");
		registerTransition("RunningLeft", "ATTACK_PRESSED", "Attack01");
		registerTransition("RunningLeft", "IN_AIR", "Falling");
		registerTransition("RunningLeft", "DEATH", "Dead");

		registerTransition("RunningRight", "RIGHT_RELEASED", "Idle");
		registerTransition("RunningRight", "LEFT_PRESSED", "RunningLeft");
		registerTransition("RunningRight", "JUMP_PRESSED", "Rising");
		registerTransition("RunningRight", "ATTACK_PRESSED", "Attack01");
		registerTransition("RunningRight", "IN_AIR", "Falling");
		registerTransition("RunningRight", "DEATH", "Dead");

		registerTransition("Attack01", EVT_STATE_END, "Idle");
		registerTransition("Attack01", "ATTACK_PRESSED", "Attack02");
		registerTransition("Attack01", "DEATH", "Dead");
		registerTransition("Attack02", EVT_STATE_END, "Idle");
		registerTransition("Attack02", "DEATH", "Dead");

		this->toJSON(transitionsFilePath);
	}
}

void Character::_setupCollisionHandler() {

	// I think each state should have its own collision handler?
	_collisionEventHandler = [=](const Event* e) {

		CollisionEvent* collisionEvent = (CollisionEvent*)e;

		GameObject* otherObject = collisionEvent->involvedObject;
		if (otherObject->getType() == GAME_OBJ_TILE) {

			Tile* tile = (Tile*)otherObject;

			if (this->_tile == tile) {
				return; // Already on this tile, no need to handle again
			}

			vector2 directionToObject = tile->getPosition() - this->getPosition();

			// Check if the other tile is closer than the one we are currently on
			if (this->_tile) {
				vector2 currentDirectionToObject = this->_tile->getPosition() - this->getPosition();
				if (currentDirectionToObject.norm() < directionToObject.norm()) {
					return; // The current tile is closer, no need to handle this one
				}
			}

			directionToObject.normalize();

			this->_tile = tile;

#if _DEBUG
			if (DEBUGGING && Debug::dbgCollision)
			{
				char buffer[256];
				sprintf_s(buffer, sizeof(buffer), "Tile (%i):\n\tpos{ % f,% f }\n", this->_tile->getTileIndex(), this->_tile->_position.x, this->_tile->_position.y);
				DEBUG_MSG(buffer);

				if (Renderable* renderable = this->_tile->getRenderable()) {
					sprintf_s(buffer, sizeof(buffer), "\trenderablePos{%f, %f}\n", renderable->getPosition().x, renderable->getPosition().y);
					DEBUG_MSG(buffer);
				}

				if (Collidable* collidable = this->_tile->getCollidable()) {
					switch (collidable->getType()) {
					case COL_OBJ_SQUARE: {
						Square* square = (Square*)otherObject->getCollidable();
						sprintf_s(buffer, sizeof(buffer), "\tcolSquare{%f, %f, %f, %f}\n", square->_x, square->_y, square->getMax().x, square->getMax().y);
						DEBUG_MSG(buffer);
						break;
					}
					case COL_OBJ_VOID:
						break;
					case COL_OBJ_CIRCLE:
						break;
					case COL_OBJ_PLANE:
						break;
					case COL_OBJ_GROUP:
						break;
					}
				}
			}
#endif
			if (this->_tile->getTileType() == "tile") {

				Square* square = NULL;
				Square* otherSquare = NULL;

				if (Collidable* collidable = this->getCollidable())
				{
					if (this->getCollidable()->getType() != COL_OBJ_SQUARE) {
						DEBUG_MSG("Character collidable is not a square!\n");
						return;
					}

					square = (Square*)this->getCollidable();

					if (otherObject->getCollidable()->getType() != COL_OBJ_SQUARE) {
						DEBUG_MSG("Other object collidable is not a square!\n");
						return;
					}
					otherSquare = (Square*)this->_tile->getCollidable();
				}

				// Determine if we are colliding from the top or bottom
				if (directionToObject.y < 0.0f) {
					this->sendInput("JUMP_RELEASED");
				}
				else if (directionToObject.y > 0.0f) {
					this->sendInput("GROUND_COLLISION");
				}

				if (square != NULL && otherSquare != NULL)
				{
					// Determine if we are colliding from the left or right
					if (directionToObject.x < 0.0f) {
						// Stop from moving
						// this->addImpulse(-this->getState()->getDirection(), this->getState()->getForce());
					}
					else if (directionToObject.x > 0.0f) {
						//this->addImpulse(vector2(-1.0f, 0.0f), 1.0f); // move to the left?
						//this->setPosition(otherSquare->getPosition().x + otherSquare->getMax().x, this->getPosition().y);
					}
				}
			}
			else if (this->_tile->getTileType() == "key")
			{
				this->_tile->getRenderable()->setVisibility(false);
				this->_tile->getCollidable()->setActive(false);
				this->sendInput("DEATH");
			}
		}
	};
}

void Character::update(float time)
{
	GameObject::update(time);
	GameObjectState* state = this->getState();

	if (state) {

		const char* stateName = state->getName();

		if (Player* player = Engine2D::getGame()->getPlayerWith(this)) {
#if _DEBUG
			if (KEYBOARD) {
				if (Engine2D::getInput()->getKeyboard()->keyPressed(KEYBOARD->getKeys().KBK_F)) {
					this->sendInput("DEATH");
				}
			}
#endif
			if (!strcmp(stateName, "Jump") || !strcmp(stateName, "Falling"))
			{
				this->_tile = NULL; // Reset the tile we are on, so we can check for collisions again

				if (player->getController()->getAction("LEFT")->isActive()) {
					this->setPosition(this->getPosition().x - MOVE_UNITS * time, this->getPosition().y);
					this->getRenderable()->setScale(-abs(this->getRenderable()->getScale().x), this->getRenderable()->getScale().y);
				}
				else if (player->getController()->getAction("RIGHT")->isActive()) {
					this->setPosition(this->getPosition().x + MOVE_UNITS * time, this->getPosition().y);
					this->getRenderable()->setScale(abs(this->getRenderable()->getScale().x), this->getRenderable()->getScale().y);
				}
			}

			// Having this idea about conditional state changes,
			// like having a "KEEP_ALIVE" condition in the event queue, and in the lack of that condition,
			// the character will fall or otherwise change state

			if (!this->_tile) {
				if (!containsCondition("GROUND_COLLISION")) {
					if (!strcmp(stateName, "Idle") || !strcmp(stateName, "RunningLeft") || !strcmp(stateName, "RunningRight"))
						this->sendInput("");// IN_AIR
				}
			}
			else {
				// Check if we're still colliding with the tile
				if (Collidable* collidable = this->getCollidable()) {
					if (!collidable->collidesWith(this->_tile->getCollidable())) {
						_tile = NULL;
					}
				}
				else
					_tile = NULL; // No collidable, so we can't check for collisions
			}
		} // if (Player)
	}
#if _DEBUG
	if (DEBUGGING && Debug::dbgObjects)
	{
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Character:\n\tpos{ % f,% f }\n", this->_position.x, this->_position.y);
		DEBUG_MSG(buffer);

		if (Renderable* renderable = this->getRenderable()) {
			sprintf_s(buffer, sizeof(buffer), "\trenderablePos{%f, %f}\n", renderable->getPosition().x, renderable->getPosition().y);
			DEBUG_MSG(buffer);
		}

		if (Collidable* collidable = this->getCollidable()) {
			switch (collidable->getType()) {
			case COL_OBJ_SQUARE: {
				Square* square = (Square*)this->getCollidable();
				sprintf_s(buffer, sizeof(buffer), "\tcolSquare{%f, %f, %f, %f}\n", square->_x, square->_y, square->getMax().x, square->getMax().y);
				DEBUG_MSG(buffer);
				break;
			}
			case COL_OBJ_VOID:
				break;
			case COL_OBJ_CIRCLE:
				break;
			case COL_OBJ_PLANE:
				break;
			case COL_OBJ_GROUP:
				break;
			}
		}
	}
#endif
}
