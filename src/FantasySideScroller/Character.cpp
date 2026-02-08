// Character.cpp

#include "Character.h"

#include "../CollidableGroup.h"
#include "../GameState.h"
#include "../Polygon.h"
#include "../Square.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <vector>

namespace {
constexpr float kGroundLossGraceSeconds = 0.06f;
constexpr float kDropThroughDurationSeconds = 0.20f;
constexpr float kSupportSampleInset = 2.0f;
constexpr float kDefaultMaxSnapPerFrame = 8.0f;
constexpr float kGroundNormalThreshold = 0.2f;
constexpr float kOneWayTopApproachEpsilon = 1.0f;
constexpr float kLocomotionFootLocalY = 22.0f;
constexpr float kFootlineTolerance = 0.5f;
constexpr float kFootlineEpsilon = 0.001f;
}

Character::Character(void) : 
	GameObject(GAME_OBJ_OBJECT),
	_tile(NULL) {

	// TODO: Write a loadObjectFromJSON function to load the character from a JSON file, otherwise, call the init functions directly
	//		 Basically, it calls

	_initStates();
	_initTransitions();

	this->setBuffered(false);
	this->setState("Falling");
	this->setMass(100);
}

Character::~Character() {}

bool Character::_isOneWayTile(const Tile* tile) const
{
	return tile &&
		tile->getTileType() == "tile" &&
		tile->isOneWay();
}

bool Character::_isDropThroughRequested() const
{
	Game* game = Engine2D::getGame();
	if (!game) {
		return false;
	}

	Player* player = game->getPlayerWith((GameObject*)this);
	if (!player || !player->getController()) {
		return false;
	}

	Controller* controller = player->getController();
	Action* jumpAction = controller->getAction("JUMP");
	Action* downAction = controller->getAction("DOWN");
	if (!jumpAction || !downAction) {
		return false;
	}

	return controller->buttonPressed(jumpAction) && controller->buttonDown(downAction);
}

void Character::_startDropThrough()
{
	_dropThroughTimer = kDropThroughDurationSeconds;

	for (auto itr = _groundContacts.begin(); itr != _groundContacts.end();) {
		Tile* tile = *itr;
		if (_isOneWayTile(tile)) {
			itr = _groundContacts.erase(itr);
			continue;
		}
		++itr;
	}

	_refreshGroundTile();
	_timeWithoutGroundContact = kGroundLossGraceSeconds;

	if (GameObjectState* state = this->getState()) {
		const char* stateName = state->getName();
		if (_isGroundedLocomotionState(stateName) || !strcmp(stateName, "Attack01") || !strcmp(stateName, "Attack02")) {
			this->sendInput("IN_AIR");
		}
	}
}

bool Character::_canCollideWithOneWayTile(const Tile* tile) const
{
	if (!_isOneWayTile(tile)) {
		return true;
	}

	if (_dropThroughTimer > 0.0f) {
		return false;
	}

	if (this->getVelocity().y < 0.0f) {
		return false;
	}

	Collidable* selfCollidable = ((Character*)this)->getCollidable();
	Collidable* tileCollidable = ((Tile*)tile)->getCollidable();
	if (!selfCollidable || !tileCollidable || !selfCollidable->isActive() || !tileCollidable->isActive()) {
		return true;
	}

	if (selfCollidable->getType() != COL_OBJ_SQUARE) {
		return true;
	}

	Square* bodySquare = (Square*)selfCollidable;
	const vector2 bodyMin = bodySquare->getMin();
	const vector2 bodyMax = bodySquare->getMax();
	const float sampleX = bodyMin.x + ((bodyMax.x - bodyMin.x) * 0.5f);

	float supportY = 0.0f;
	if (!_sampleSupportY(tileCollidable, sampleX, supportY)) {
		return true;
	}

	const float bodyTop = bodyMin.y;
	return bodyTop < (supportY + kOneWayTopApproachEpsilon);
}

bool Character::shouldCollideWith(const GameObject& other) const
{
	if (!GameObject::shouldCollideWith(other)) {
		return false;
	}

	if (other.getType() != GAME_OBJ_TILE) {
		return true;
	}

	const Tile* tile = (const Tile*)(&other);
	if (!tile) {
		return false;
	}

	if (tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile)) {
		return _canCollideWithOneWayTile(tile);
	}

	return true;
}

bool Character::_isGroundContact(const CollisionContact& contact) const
{
	if (!contact.other || contact.phase == CollisionPhase::Exit || contact.other->getType() != GAME_OBJ_TILE) {
		return false;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile || tile->getTileType() != "tile") {
		return false;
	}

	if (tile->isNonCollidingLayer()) {
		return false;
	}

	if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
		return false;
	}

	if (contact.normal.has_value()) {
		return contact.normal->y > kGroundNormalThreshold;
	}

	vector2 directionToObject = tile->getPosition() - this->getPosition();
	directionToObject.normalize();
	return directionToObject.y > 0.0f;
}

bool Character::_isGroundedLocomotionState(const char* stateName) const
{
	if (!stateName) {
		return false;
	}

	return !strcmp(stateName, "Idle") ||
		!strcmp(stateName, "RunningLeft") ||
		!strcmp(stateName, "RunningRight") ||
		!strcmp(stateName, "Landing");
}

bool Character::_getStateFootLocalY(const GameObjectState* state, float& outFootY) const
{
	outFootY = 0.0f;
	if (!state) {
		return false;
	}

	const GameObjectState* resolvedState = state;
	Collidable* stateCollidable = const_cast<GameObjectState*>(resolvedState)->getCollidable();
	if (!stateCollidable) {
		return false;
	}

	std::function<bool(const Collidable*, float, float&)> findFootLocalY =
		[&](const Collidable* collidable, float parentOffsetY, float& outFootLocalY) -> bool {
			if (!collidable) {
				return false;
			}

			switch (collidable->getType()) {
			case COL_OBJ_SQUARE: {
				const Square* square = (const Square*)collidable;
				if (!square) {
					return false;
				}

				outFootLocalY = parentOffsetY + square->getPosition().y + square->getHeight();
				return true;
			}
			case COL_OBJ_POLYGON: {
				const PolygonCollider* polygon = (const PolygonCollider*)collidable;
				if (!polygon || !polygon->isValid()) {
					return false;
				}

				const std::vector<vector2>& localVertices = polygon->getLocalVertices();
				if (localVertices.empty()) {
					return false;
				}

				float maxVertexY = std::numeric_limits<float>::lowest();
				for (const vector2& vertex : localVertices) {
					if (vertex.y > maxVertexY) {
						maxVertexY = vertex.y;
					}
				}

				outFootLocalY = parentOffsetY + polygon->getPosition().y + maxVertexY;
				return true;
			}
			case COL_OBJ_GROUP: {
				const CollidableGroup* group = (const CollidableGroup*)collidable;
				if (!group) {
					return false;
				}

				const float groupOffsetY = parentOffsetY + group->getPosition().y;
				bool hasMember = false;
				float bestMemberFootY = std::numeric_limits<float>::lowest();
				for (const Collidable* member : *group) {
					float memberFootY = 0.0f;
					if (!findFootLocalY(member, groupOffsetY, memberFootY)) {
						continue;
					}

					if (!hasMember || memberFootY > bestMemberFootY) {
						bestMemberFootY = memberFootY;
						hasMember = true;
					}
				}

				if (!hasMember) {
					return false;
				}

				outFootLocalY = bestMemberFootY;
				return true;
			}
			default:
				return false;
			}
		};

	return findFootLocalY(stateCollidable, 0.0f, outFootY);
}

void Character::_refreshGroundTile()
{
	Tile* bestTile = NULL;
	float bestDistance = std::numeric_limits<float>::max();
	std::vector<Tile*> staleTiles;

	for (Tile* tile : _groundContacts) {
		if (!tile || tile->getTileType() != "tile") {
			staleTiles.push_back(tile);
			continue;
		}

		if (tile->isNonCollidingLayer()) {
			staleTiles.push_back(tile);
			continue;
		}

		if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
			staleTiles.push_back(tile);
			continue;
		}

		Collidable* collidable = tile->getCollidable();
		if (!collidable || !collidable->isActive()) {
			staleTiles.push_back(tile);
			continue;
		}

		vector2 delta(
			tile->getPosition().x - this->getPosition().x,
			tile->getPosition().y - this->getPosition().y);
		float distance = delta.norm();
		if (distance < bestDistance) {
			bestDistance = distance;
			bestTile = tile;
		}
	}

	for (Tile* staleTile : staleTiles) {
		_groundContacts.erase(staleTile);
	}

	_tile = bestTile;
}

bool Character::_sampleSupportY(const Collidable* collidable, float sampleX, float& outY) const
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	constexpr float kHorizontalEpsilon = 0.001f;
	switch (collidable->getType()) {
	case COL_OBJ_SQUARE: {
		const Square* square = (const Square*)collidable;
		if (!square) {
			return false;
		}

		const vector2 min = square->getMin();
		const vector2 max = square->getMax();
		if (sampleX < (min.x - kHorizontalEpsilon) || sampleX > (max.x + kHorizontalEpsilon)) {
			return false;
		}

		outY = min.y;
		return true;
	}
	case COL_OBJ_POLYGON: {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (!polygon || !polygon->isValid()) {
			return false;
		}
		return polygon->findTopSurfaceYAtX(sampleX, outY);
	}
	case COL_OBJ_GROUP: {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group) {
			return false;
		}

		bool found = false;
		float bestY = std::numeric_limits<float>::max();
		for (const Collidable* member : *group) {
			float memberY = 0.0f;
			if (!_sampleSupportY(member, sampleX, memberY)) {
				continue;
			}

			if (!found || memberY < bestY) {
				bestY = memberY;
				found = true;
			}
		}

		if (!found) {
			return false;
		}

		outY = bestY;
		return true;
	}
	default:
		return false;
	}
}

Tile* Character::_findGroundSupportTile(float footY, float maxSnapDistance, float& outSupportY)
{
	Collidable* body = this->getCollidable();
	if (!body || !body->isActive() || body->getType() != COL_OBJ_SQUARE) {
		return NULL;
	}

	Square* bodySquare = (Square*)body;
	const vector2 bodyMin = bodySquare->getMin();
	const vector2 bodyMax = bodySquare->getMax();
	const float bodyWidth = bodyMax.x - bodyMin.x;
	if (bodyWidth <= 0.0f) {
		return NULL;
	}

	float sampleInset = std::min(kSupportSampleInset, bodyWidth * 0.45f);
	if (sampleInset < 0.0f) {
		sampleInset = 0.0f;
	}

	const float sampleXs[3] = {
		bodyMin.x + sampleInset,
		bodyMin.x + (bodyWidth * 0.5f),
		bodyMax.x - sampleInset
	};

	Game* game = Engine2D::getGame();
	if (!game || game->empty()) {
		return NULL;
	}

	ProgramState* activeProgramState = game->top();
	GameState* activeGameState = dynamic_cast<GameState*>(activeProgramState);
	if (!activeGameState) {
		return NULL;
	}

	const auto& objects = activeGameState->getObjectManager()->getObjects();
	Tile* bestUpwardSupportTile = NULL;
	float bestUpwardSupportY = footY;
	float bestUpwardDistance = std::numeric_limits<float>::max();
	Tile* bestDownwardSupportTile = NULL;
	float bestDownwardSupportY = footY;
	float bestDownwardDelta = std::numeric_limits<float>::max();

	for (const auto& entry : objects) {
		GameObject* object = entry.second;
		if (!object || object == this || object->getType() != GAME_OBJ_TILE) {
			continue;
		}

		Tile* tile = (Tile*)object;
		if (!tile || tile->getTileType() != "tile") {
			continue;
		}

		if (tile->isNonCollidingLayer()) {
			continue;
		}

		if (_isOneWayTile(tile) && !_canCollideWithOneWayTile(tile)) {
			continue;
		}

		Collidable* tileCollidable = tile->getCollidable();
		if (!tileCollidable || !tileCollidable->isActive()) {
			continue;
		}

		for (float sampleX : sampleXs) {
			float supportY = 0.0f;
			if (!_sampleSupportY(tileCollidable, sampleX, supportY)) {
				continue;
			}

			const float deltaY = supportY - footY;
			if (deltaY < -maxSnapDistance || deltaY > maxSnapDistance) {
				continue;
			}

			// Prefer supports that resolve penetration (support at/above current footline)
			// before supports that move the character farther downward.
			if (deltaY <= 0.0f) {
				const float distance = std::fabs(deltaY);
				if (distance < bestUpwardDistance) {
					bestUpwardDistance = distance;
					bestUpwardSupportY = supportY;
					bestUpwardSupportTile = tile;
				}
			}
			else if (deltaY < bestDownwardDelta) {
				bestDownwardDelta = deltaY;
				bestDownwardSupportY = supportY;
				bestDownwardSupportTile = tile;
			}
		}
	}

	if (bestUpwardSupportTile) {
		outSupportY = bestUpwardSupportY;
		return bestUpwardSupportTile;
	}

	if (bestDownwardSupportTile) {
		outSupportY = bestDownwardSupportY;
		return bestDownwardSupportTile;
	}

	return NULL;
}

void Character::_initStates() {
	std::string animationsFilePath = BasePath("Character/Animations.json");
	std::vector<Animation*> loadedAnimations;
	bool animationsFromJson = false;

	if (FileSystem::FileExists(animationsFilePath)) {
		loadedAnimations = Animations::fromJSON(animationsFilePath.c_str(), nullptr);
		if (!loadedAnimations.empty()) {
			for (Animation* animation : loadedAnimations) {
				if (animation) {
					_animationManager.store(animation);
				}
			}
		}
		animationsFromJson = !loadedAnimations.empty();
	}

	auto findLoadedAnimation = [&](const char* name) -> Animation* {
		for (Animation* animation : loadedAnimations) {
			if (animation && std::strcmp(animation->getName(), name) == 0) {
				return animation;
			}
		}
		return nullptr;
	};

	auto getAnimation = [&](const char* name, bool& loaded) -> Animation* {
		Animation* animation = animationsFromJson ? findLoadedAnimation(name) : nullptr;
		loaded = (animation != nullptr);
		if (!animation) {
			animation = _animationManager.create();
		}
		return animation;
	};

	GameObjectState* idle = this->addState("Idle");

	bool idleLoaded = false;
	Animation* idleAnimation = getAnimation("Idle", idleLoaded);

	vector2 idleFrameDimensions{ 64, 80 };

	if (!idleLoaded) {
		Texture* idleSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Idle/Idle-Sheet.png").c_str());
		Animations::createFramesForAnimation(idleAnimation, idleSheet, idleFrameDimensions, _spriteManager);
		idleAnimation->setMode(Animation::Mode::eOscillate);
		idleAnimation->setFrameRate(30);
	}

	idleAnimation->setName(idle->getName());
	idleAnimation->center();

	idle->setPreserveScaling(true);
	idle->setRenderable(idleAnimation);

	static Square idleHitBox({ -10, kLocomotionFootLocalY - 48.0f }, 20, 48);
	for (unsigned int i = 0; i < idleAnimation->getFrameCount(); i++) {
		(*idleAnimation)[i]->setCollidable(&idleHitBox);
	}

	/////////////////////////////////////////
	GameObjectState* rising = this->addState("Rising");
	rising->setPreserveScaling(true);
	rising->setExecuteTime(0.1);
	//rising->setDirection(vector2(0.0f, -1.0f));
	//rising->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.1));

	bool risingLoaded = false;
	Animation* risingAnimation = getAnimation("Rising", risingLoaded);
	vector2 risingDimensions{ 64, 64 };
	if (!risingLoaded) {
		Texture* risingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-Start/Jump-Start-Sheet.png").c_str());
		Animations::createFramesForAnimation(risingAnimation, risingSheet, risingDimensions, _spriteManager);
		risingAnimation->setFrameRate(30);
	}

	risingAnimation->setName(rising->getName());
	risingAnimation->setOffset({ 0.0, -8.0 });
	risingAnimation->center();

	rising->setRenderable(risingAnimation);

	static Square risingHitBox({ -14, kLocomotionFootLocalY - 52.0f }, 20, 52);
	for (unsigned int i = 0; i < risingAnimation->getFrameCount(); i++) {
		(*risingAnimation)[i]->setCollidable(&risingHitBox);
	}

	/////////////////////////////////////////

	GameObjectState* jump = this->addState("Jump");
	jump->setPreserveScaling(true);
	jump->setExecuteTime(0.2);
	jump->setDirection(vector2(0.0f, -1.0f));
	jump->setForce(MOVE_UNITS * (JUMP_MULTIPLIER * 0.67));

	bool jumpLoaded = false;
	Animation* jumpAnimation = getAnimation("Jump", jumpLoaded);

	vector2 jumpDimensions{ 64, 64 };

	if (!jumpLoaded) {
		Texture* jumpSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jumlp-All/Jump-All-Sheet.png").c_str());
		Animations::createFramesForAnimation(jumpAnimation, jumpSheet, jumpDimensions, _spriteManager, 4, 8);
		jumpAnimation->setMode(Animation::Mode::eOscillate);
		jumpAnimation->setFrameRate(60);
	}

	jumpAnimation->setName(jump->getName());
	jumpAnimation->setOffset({ 0.0, -8.0 });
	jumpAnimation->center();

	jump->setRenderable(jumpAnimation);

	static Square jumpHitBox({ -14, kLocomotionFootLocalY - 52.0f }, 20, 52);
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

	bool landingLoaded = false;
	Animation* landingAnimation = getAnimation("Landing", landingLoaded);
	landingAnimation->setName(landing->getName());
	landingAnimation->setOffset({ 0.0, -8.0 });
	landing->setRenderable(landingAnimation);

	if (!landingLoaded) {
		Texture* landingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-End/Jump-End-Sheet.png").c_str());
		Animations::createFramesForAnimation(landingAnimation, landingSheet, landingDimensions, _spriteManager);
		landingAnimation->setFrameRate(30);
		landingAnimation->setSpeed(2.7f);
	}

	for (unsigned int i = 0; i < landingAnimation->getFrameCount(); i++) {
		(*landingAnimation)[i]->setCollidable(&jumpHitBox);
	}
	landingAnimation->center();

	/////////////////////////////////////////

	GameObjectState* runningLeft = this->addState("RunningLeft");
	GameObjectState* runningRight = this->addState("RunningRight");

	vector2 runningDimensions{ 80.0, 80.0 };

	runningLeft->setDirection({ -1.0, 0.0 });
	runningRight->setDirection({ 1.0, 0.0 });

	runningLeft->setForce(100);
	runningRight->setForce(100);

	bool runningLeftLoaded = false;
	bool runningRightLoaded = false;
	Animation* runningLeftAnimation = getAnimation("RunningLeft", runningLeftLoaded);
	Animation* runningRightAnimation = getAnimation("RunningRight", runningRightLoaded);

	runningLeftAnimation->setName(runningLeft->getName());
	runningRightAnimation->setName(runningRight->getName());

	runningLeft->setRenderable(runningLeftAnimation);
	runningRight->setRenderable(runningRightAnimation);

	if (!runningLeftLoaded || !runningRightLoaded) {
		Texture* runningSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Run/Run-Sheet.png").c_str());
		if (!runningLeftLoaded) {
			Animations::createFramesForAnimation(runningLeftAnimation, runningSheet, runningDimensions, _spriteManager);
		}
		if (!runningRightLoaded) {
			Animations::createFramesForAnimation(runningRightAnimation, runningSheet, runningDimensions, _spriteManager);
		}
	}

	static Square runHitBox({ -10, kLocomotionFootLocalY - 42.0f }, 26, 42);
	for (unsigned int i = 0; i < runningRightAnimation->getFrameCount(); i++) {
		(*runningLeftAnimation)[i]->setCollidable(&runHitBox);
		(*runningRightAnimation)[i]->setCollidable(&runHitBox);
	}

	runningLeftAnimation->mirror(true, false);
	runningLeftAnimation->center();

	runningRightAnimation->center();
	if (!runningLeftLoaded) {
		runningLeftAnimation->setMode(Animation::Mode::eLoop);
		runningLeftAnimation->setFrameRate(60);
		runningLeftAnimation->setSpeed(1.1f);
	}
	if (!runningRightLoaded) {
		runningRightAnimation->setMode(Animation::Mode::eLoop);
		runningRightAnimation->setFrameRate(60);
		runningRightAnimation->setSpeed(1.1f);
	}

	/////////////////////////////////////////

	GameObjectState* attack01 = this->addState("Attack01");
	GameObjectState* attack02 = this->addState("Attack02");

	attack01->setPreserveScaling(true);
	attack02->setPreserveScaling(true);

	vector2 attackDimensions{ 96.0, 80.0 };

	Texture* attackSheet = nullptr;
	bool attack01Loaded = false;
	bool attack02Loaded = false;
	Animation* attack01Animation = getAnimation("Attack01", attack01Loaded);
	if (!attack01Loaded || !attack02Loaded) {
		attackSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Attack-01/Attack-01-Sheet.png").c_str());
	}
	if (!attack01Loaded) {
		Animations::createFramesForAnimation(attack01Animation, attackSheet, attackDimensions, _spriteManager, 0, 5);
		attack01Animation->setFrameRate(60);
	}
	attack01Animation->setName(attack01->getName());
	attack01Animation->center();
	attack01->setRenderable(attack01Animation);

	Animation* attack02Animation = getAnimation("Attack02", attack02Loaded);
	if (!attack02Loaded) {
		Animations::createFramesForAnimation(attack02Animation, attackSheet, attackDimensions, _spriteManager, 5, 3);
		attack02Animation->setFrameRate(30);
	}
	attack02Animation->setName(attack02->getName());
	attack02Animation->center();
	attack02->setRenderable(attack02Animation);

	/////////////////////////////////////////

	GameObjectState* dead = this->addState("Dead");
	dead->setPreserveScaling(true);

	vector2 deadDimensions{ 80, 64 };

	Texture* deadSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Dead/Dead-Sheet.png").c_str());

	bool deadLoaded = false;
	Animation* deadAnimation = getAnimation("Dead", deadLoaded);
	if (!deadLoaded) {
		Animations::createFramesForAnimation(deadAnimation, deadSheet, deadDimensions, _spriteManager);
		deadAnimation->setFrameRate(30);
	}

	deadAnimation->setName(dead->getName());
	deadAnimation->setOffset({ 8.0, 8.0 });
	deadAnimation->center();

	dead->setRenderable(deadAnimation);

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision) {
		struct FootlineCheck {
			const char* stateName;
			GameObjectState* state;
		};

		const FootlineCheck checks[] = {
			{ "Idle", idle },
			{ "Rising", rising },
			{ "Jump", jump },
			{ "Landing", landing },
			{ "RunningLeft", runningLeft }
		};

		char buffer[256];
		for (const FootlineCheck& check : checks) {
			float footLocalY = 0.0f;
			if (!_getStateFootLocalY(check.state, footLocalY)) {
				sprintf_s(buffer, sizeof(buffer), "Character Footline [%s]: unavailable\n", check.stateName);
				DEBUG_MSG(buffer);
				continue;
			}

			sprintf_s(buffer, sizeof(buffer), "Character Footline [%s]: %.2f (target %.2f)\n",
				check.stateName, footLocalY, kLocomotionFootLocalY);
			DEBUG_MSG(buffer);

			if (std::fabs(footLocalY - kLocomotionFootLocalY) > kFootlineTolerance) {
				sprintf_s(buffer, sizeof(buffer),
					"WARNING Character Footline mismatch [%s]: %.2f vs %.2f\n",
					check.stateName, footLocalY, kLocomotionFootLocalY);
				DEBUG_MSG(buffer);
			}
		}
	}
#endif

	// This isn't really used for much at the moment...
	// But I'd like to eventually store the collision data from each of the frames
	if (!animationsFromJson) {
		std::vector<Animation*> animations;
		for (auto& animation : _animationManager) {
			if (animation) {
				animations.push_back(animation);
			}
		}
		Animations::toJSON(animations, animationsFilePath.c_str());
	}
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

void Character::onStateDidEnter(State* previous, State* current)
{
	GameObject::onStateDidEnter(previous, current);
	_pendingTransitionFootCorrection = 0.0f;

	GameObjectState* previousState = (GameObjectState*)previous;
	GameObjectState* currentState = (GameObjectState*)current;
	if (!previousState || !currentState) {
		return;
	}

	float previousFootLocalY = 0.0f;
	float currentFootLocalY = 0.0f;
	if (!_getStateFootLocalY(previousState, previousFootLocalY) ||
		!_getStateFootLocalY(currentState, currentFootLocalY)) {
		return;
	}

	const float deltaY = previousFootLocalY - currentFootLocalY;
	if (std::fabs(deltaY) <= kFootlineEpsilon) {
		return;
	}

	this->setPosition(this->getPosition().x, this->getPosition().y + deltaY);
	_pendingTransitionFootCorrection = std::fabs(deltaY);
}

void Character::handleCollisionContact(const CollisionContact& contact)
{
	if (!contact.other || contact.other->getType() != GAME_OBJ_TILE) {
		return;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile) {
		return;
	}

	if (contact.phase == CollisionPhase::Exit) {
		_groundContacts.erase(tile);
		_refreshGroundTile();
		return;
	}

	if (_isGroundContact(contact)) {
		_groundContacts.insert(tile);
		_timeWithoutGroundContact = 0.0f;
		_refreshGroundTile();
	}

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision)
	{
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer), "Tile (%i):\n\tpos{ % f,% f }\n", tile->getTileIndex(), tile->_position.x, tile->_position.y);
		DEBUG_MSG(buffer);

		if (Renderable* renderable = tile->getRenderable()) {
			sprintf_s(buffer, sizeof(buffer), "\trenderablePos{%f, %f}\n", renderable->getPosition().x, renderable->getPosition().y);
			DEBUG_MSG(buffer);
		}

		if (Collidable* collidable = tile->getCollidable()) {
			switch (collidable->getType()) {
			case COL_OBJ_SQUARE: {
				Square* square = (Square*)contact.other->getCollidable();
				sprintf_s(buffer, sizeof(buffer), "\tcolSquare{%f, %f, %f, %f}\n", square->_x, square->_y, square->getMax().x, square->getMax().y);
				DEBUG_MSG(buffer);
				break;
			}
			case COL_OBJ_POLYGON:
				DEBUG_MSG("\tcolPolygon\n");
				break;
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

	if (tile->getTileType() == "key" && contact.phase == CollisionPhase::Enter)
	{
		if (Renderable* renderable = tile->getRenderable()) {
			renderable->setVisibility(false);
		}
	}
}

const char* Character::mapCollisionToCommand(const CollisionContact& contact) const
{
	if (!contact.other || contact.phase == CollisionPhase::Exit || contact.other->getType() != GAME_OBJ_TILE) {
		return NULL;
	}

	Tile* tile = (Tile*)contact.other;
	if (!tile) {
		return NULL;
	}

	if (tile->getTileType() == "key") {

		if (Collidable* collidable = tile->getCollidable()) {	

			if (tile->getLayerCollisionMode() != TileCollisionMode::None && contact.phase == CollisionPhase::Enter) 
			{
				tile->setLayerCollisionMode(TileCollisionMode::None);
				return "DEATH";
			}
		}
	}

	if (tile->getTileType() != "tile") {
		return NULL;
	}

	if (contact.normal) {
		if (contact.normal->y < -0.5f) {
			return "JUMP_RELEASED";
		}
		if (contact.normal->y > kGroundNormalThreshold) {
			if (contact.phase == CollisionPhase::Enter) {
				return "GROUND_COLLISION";
			}
			if (contact.phase == CollisionPhase::Stay) {
				if (GameObjectState* state = this->getState()) {
					if (!strcmp(state->getName(), "Falling")) {
						return "GROUND_COLLISION";
					}
				}
			}
		}
	}
	else {
		vector2 directionToObject = tile->getPosition() - this->getPosition();
		directionToObject.normalize();
		if (directionToObject.y < -0.5f) {
			return "JUMP_RELEASED";
		}
		if (directionToObject.y > kGroundNormalThreshold) {
			if (contact.phase == CollisionPhase::Enter) {
				return "GROUND_COLLISION";
			}
			if (contact.phase == CollisionPhase::Stay) {
				if (GameObjectState* state = this->getState()) {
					if (!strcmp(state->getName(), "Falling")) {
						return "GROUND_COLLISION";
					}
				}
			}
		}
	}

	return NULL;
}

void Character::update(float time)
{
	GameObject::update(time);

	if (_dropThroughTimer > 0.0f) {
		_dropThroughTimer = std::max(0.0f, _dropThroughTimer - time);
	}

	_refreshGroundTile();

	if (_isDropThroughRequested()) {
		bool groundedOnOneWay = _isOneWayTile(_tile);

		if (!groundedOnOneWay) {
			for (Tile* contactTile : _groundContacts) {
				if (_isOneWayTile(contactTile) && _canCollideWithOneWayTile(contactTile)) {
					groundedOnOneWay = true;
					break;
				}
			}
		}

		if (groundedOnOneWay) {
			_startDropThrough();
		}
	}

	float baseSnapPerFrame = kDefaultMaxSnapPerFrame;
	if (_tile && _tile->getTileSet()) {
		baseSnapPerFrame = std::max(1.0f, _tile->getTileSet()->getTileSize() * 0.5f);
	}
	float maxSnapPerFrame = baseSnapPerFrame;
	if (_pendingTransitionFootCorrection > 0.0f) {
		maxSnapPerFrame = std::max(baseSnapPerFrame, _pendingTransitionFootCorrection + 1.0f);
	}

	float footY = this->getPosition().y;
	if (Collidable* bodyCollidable = this->getCollidable()) {
		if (bodyCollidable->getType() == COL_OBJ_SQUARE) {
			Square* bodySquare = (Square*)bodyCollidable;
			footY = bodySquare->getMax().y;
		}
	}

	float supportY = footY;
	Tile* supportTile = _findGroundSupportTile(footY, maxSnapPerFrame, supportY);
	bool hasGroundSupport = (supportTile != NULL);
	if (supportTile) {
		_tile = supportTile;
	}

	GameObjectState* state = this->getState();

	if (!hasGroundSupport) {
		_timeWithoutGroundContact += time;
	}
	else {
		_timeWithoutGroundContact = 0.0f;
	}

	if (state) {

		const char* stateName = state->getName();
		const bool groundedLocomotionState = _isGroundedLocomotionState(stateName);
		const bool shouldApplyGroundSnap =
			groundedLocomotionState ||
			!strcmp(stateName, "Attack01") ||
			!strcmp(stateName, "Attack02");

		if (hasGroundSupport && shouldApplyGroundSnap) {
			float deltaY = supportY - footY;
			if (std::fabs(deltaY) > 0.001f) {
				deltaY = std::max(-maxSnapPerFrame, std::min(maxSnapPerFrame, deltaY));
				this->setPosition(this->getPosition().x, this->getPosition().y + deltaY);
				footY += deltaY;
			}
		}

		if (Player* player = Engine2D::getGame()->getPlayerWith(this)) {
//#if _DEBUG
			if (KEYBOARD) {
				if (Engine2D::getInput()->getKeyboard()->keyPressed(KEYBOARD->getKeys().KBK_F)) {
					this->sendInput("DEATH");
				}
			}
//#endif
			if (!strcmp(stateName, "Jump") || !strcmp(stateName, "Falling"))
			{
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

			// Grounded state is tracked by collision Enter/Stay/Exit callbacks.
			// Debounce loss slightly to avoid one-frame Enter/Exit jitter.
			if (!hasGroundSupport && _timeWithoutGroundContact >= kGroundLossGraceSeconds) {
				if (_isGroundedLocomotionState(stateName)) {
					this->sendInput("IN_AIR");
				}
			}
		} // if (Player)
	}
	_pendingTransitionFootCorrection = 0.0f;
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
			case COL_OBJ_POLYGON:
				DEBUG_MSG("\tcolPolygon\n");
				break;
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
