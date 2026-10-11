// CharacterStateSetup.cpp

#include "Character.h"
#include "CharacterTuning.h"

#include "../Animation.h"
#include "../GameState.h"
#include "../Kinematics2D.h"
#include "../Polygon.h"
#include "../Square.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

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
					for (Frame* frame : animation->getFrames()) {
						if (frame && frame->getSprite()) {
							_spriteManager.store(frame->getSprite());
						}
					}
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

	// Frames whose frame data gave them colliders keep those; the rest use the state's hit box.
	auto setFallbackHitBox = [](Animation* animation, Collidable* hitBox) {
		for (Frame* frame : animation->getFrames()) {
			if (frame && !frame->getCollidable()) {
				frame->setCollidable(hitBox);
			}
		}
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
	setFallbackHitBox(idleAnimation, &idleHitBox);

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
		risingAnimation->setOffset({ 0.0, -8.0 });
	}

	risingAnimation->setName(rising->getName());
	risingAnimation->center();

	rising->setRenderable(risingAnimation);

	static Square risingHitBox({ -14, kLocomotionFootLocalY - 52.0f }, 20, 52);
	setFallbackHitBox(risingAnimation, &risingHitBox);

	/////////////////////////////////////////

	GameObjectState* jump = this->addState("Jump");
	jump->setPreserveScaling(true);
	jump->setExecuteTime(0.50);
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
		jumpAnimation->setOffset({ 0.0, -8.0 });
	}

	jumpAnimation->setName(jump->getName());
	jumpAnimation->center();

	jump->setRenderable(jumpAnimation);

	static Square jumpHitBox({ -14, kLocomotionFootLocalY - 52.0f }, 20, 52);
	setFallbackHitBox(jumpAnimation, &jumpHitBox);

	//GameObjectState* jumpLeft = this->addState("JumpLeft");
	//GameObjectState* jumpRight = this->addState("JumpRight");

	/////////////////////////////////////////

    GameObjectState* falling = this->addState("Falling");
    falling->setPreserveScaling(true);
    falling->setDirection(vector2(0.0f, 1.0f));
    // Use zero initial force for falling; gravity and momentum are handled in update().
    falling->setForce(0.0f);

    falling->setRenderable(jumpAnimation);

	/////////////////////////////////////////

	GameObjectState* landing = this->addState("Landing");
	landing->setDirection(vector2(0.0f, 1.0f));
	landing->setPreserveScaling(true);

	vector2 landingDimensions{ 64, 64 };

	bool landingLoaded = false;
	Animation* landingAnimation = getAnimation("Landing", landingLoaded);
	landingAnimation->setName(landing->getName());
	landing->setRenderable(landingAnimation);

	if (!landingLoaded) {
		Texture* landingSheet = Engine2D::getRenderer()->createTexture(BasePath("Character/Jump-End/Jump-End-Sheet.png").c_str());
		Animations::createFramesForAnimation(landingAnimation, landingSheet, landingDimensions, _spriteManager);
		landingAnimation->setFrameRate(30);
		landingAnimation->setSpeed(2.7f);
		landingAnimation->setOffset({ 0.0, -8.0 });
	}

	setFallbackHitBox(landingAnimation, &jumpHitBox);
	landingAnimation->center();

	/////////////////////////////////////////

	GameObjectState* runningLeft = this->addState("RunningLeft");
	GameObjectState* runningRight = this->addState("RunningRight");

	vector2 runningDimensions{ 80.0, 80.0 };

	runningLeft->setDirection({ -1.0, 0.0 });
	runningRight->setDirection({ 1.0, 0.0 });

	runningLeft->setForce(0.0f);
	runningRight->setForce(0.0f);

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
	setFallbackHitBox(runningLeftAnimation, &runHitBox);
	setFallbackHitBox(runningRightAnimation, &runHitBox);

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
    // Provide a collidable for attack animations. Without a collidable the
    // character temporarily loses its collision geometry during an attack,
    // which breaks ground detection and causes the character to enter the
    // falling state after the attack finishes. We reuse a slender hit box
    // similar to idle/running states so the character continues to collide with
    // terrain while attacking. Invincibility from enemies is handled in
    // shouldCollideWith.
    static Square attackHitBox({ -10.0f, kLocomotionFootLocalY - 48.0f }, 20.0f, 48.0f);
    setFallbackHitBox(attack01Animation, &attackHitBox);

	Animation* attack02Animation = getAnimation("Attack02", attack02Loaded);
	if (!attack02Loaded) {
		Animations::createFramesForAnimation(attack02Animation, attackSheet, attackDimensions, _spriteManager, 5, 3);
		attack02Animation->setFrameRate(30);
	}
	attack02Animation->setName(attack02->getName());
	attack02Animation->center();
	attack02->setRenderable(attack02Animation);
    // Apply the same hit box to the secondary attack animation frames.
    setFallbackHitBox(attack02Animation, &attackHitBox);

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
		deadAnimation->setOffset({ 8.0, 8.0 });
	}

	deadAnimation->setName(dead->getName());
	deadAnimation->center();

	dead->setRenderable(deadAnimation);

	/////////////////////////////////////////
	// Expanded actions, retargeted from the 2D-Pixel-Art-Character-Template by
	// tools/character_retarget.py. Their 72x72 frames put the feet on row 60, which a
	// centered frame already lines up with kLocomotionFootLocalY (Expanded/manifest.json
	// lists each sheet's FootY and the template's frame durations).

	const vector2 expandedFrameDimensions{ 72, 72 };
	auto addExpandedState = [&](const char* name, const char* sheetPath, float frameSeconds,
		Animation::Mode mode, float speed, float offsetY, Square* hitBox) -> GameObjectState* {
		GameObjectState* state = this->addState(name);
		state->setPreserveScaling(true);

		bool loaded = false;
		Animation* animation = getAnimation(name, loaded);
		if (!loaded) {
			Texture* sheet = Engine2D::getRenderer()->createTexture(BasePath(sheetPath).c_str());
			Animations::createFramesForAnimation(animation, sheet, expandedFrameDimensions, _spriteManager);
			for (unsigned int i = 0; i < animation->getFrameCount(); i++) {
				(*animation)[i]->setDuration(frameSeconds);
			}
			animation->setMode(mode);
			animation->setSpeed(speed);
			animation->setOffset({ 0.0, offsetY });
		}

		animation->setName(name);
		animation->center();
		state->setRenderable(animation);

		setFallbackHitBox(animation, hitBox);
		return state;
	};

	static Square crouchHitBox({ -10, kLocomotionFootLocalY - 34.0f }, 20, 34);
	static Square rollHitBox({ -10, kLocomotionFootLocalY - 28.0f }, 20, 28);
	static Square slideHitBox({ -12, kLocomotionFootLocalY - 24.0f }, 24, 24);

	[[maybe_unused]] GameObjectState* crouchIdle = addExpandedState("CrouchIdle", "Character/Expanded/Crouch-Idle/Crouch-Idle-Sheet.png",
		0.1f, Animation::Mode::eLoop, 1.0f, 0.0f, &crouchHitBox);
	addExpandedState("CrouchWalk", "Character/Expanded/Crouch-Walk/Crouch-Walk-Sheet.png",
		0.1f, Animation::Mode::eLoop, 1.0f, 0.0f, &crouchHitBox);
	// An animation frame lasts ceil(duration / (tick * speed)) ticks plus one tick to
	// advance. At 2.1x the template's 7 x 100 ms roll takes 28 ticks (~0.47 s at 60 Hz).
	[[maybe_unused]] GameObjectState* roll = addExpandedState("Roll", "Character/Expanded/Roll/Roll-Sheet.png",
		0.1f, Animation::Mode::eOnce, 2.1f, 0.0f, &rollHitBox);
	// One pass of the slide (8 x 75 ms at 1.25x, 40 ticks) is the whole slide: it ends
	// as kSlideDeceleration brings a sprint to a stop.
	[[maybe_unused]] GameObjectState* slide = addExpandedState("Slide", "Character/Expanded/Slide/Slide-Sheet.png",
		0.075f, Animation::Mode::eOnce, 1.25f, 0.0f, &slideHitBox);
	// Airborne like Falling, so it keeps Falling's body. The pose follows Zero's wall slide
	// (Mega Man Zero): hand and bent leg on the wall where that body ends, 6 px in front of centre.
	addExpandedState("WallSlide", "Character/Expanded/Wall-Slide/Wall-Slide-Sheet.png",
		0.1f, Animation::Mode::eLoop, 1.0f, 0.0f, &jumpHitBox);

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
		// Read-only: File::Open also asks for write access, which fails on
		// read-only storage such as an iOS app bundle and leaves no transitions.
		std::ifstream fileStream(transitionsFilePath);
		this->fromJSON(fileStream);
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
	_kinematic2DState().pendingTransitionFootCorrection = 0.0f;

	GameObjectState* previousState = (GameObjectState*)previous;
	GameObjectState* currentState = (GameObjectState*)current;
	if (!previousState || !currentState) {
		return;
	}

	const char* currentStateName = currentState->getName();
	if (!strcmp(currentStateName, "Falling")) {
		_kinematic2DState().fallingLandingDebounceTimer = kFallingLandingDebounceSeconds;
	}
	else {
		_kinematic2DState().fallingLandingDebounceTimer = 0.0f;
	}
	vector2 currentVelocity = this->getVelocity();
	bool velocityAdjusted = false;
	const bool stateHasCollisionShape = currentState->getCollidable() != NULL;

	const bool lockHorizontalVelocity =
		!strcmp(currentStateName, "Attack01") ||
		!strcmp(currentStateName, "Attack02");
	if (lockHorizontalVelocity && std::fabs(currentVelocity.x) > kHorizontalVelocityEpsilon) {
		currentVelocity.x = 0.0f;
		velocityAdjusted = true;
	}

	if (!stateHasCollisionShape &&
		(std::fabs(currentVelocity.x) > kHorizontalVelocityEpsilon ||
		 std::fabs(currentVelocity.y) > kHorizontalVelocityEpsilon)) {
		currentVelocity = vector2(0.0f, 0.0f);
		velocityAdjusted = true;
	}

	const bool resetDownwardVelocityForJumpStart =
		!strcmp(currentStateName, "Rising") ||
		!strcmp(currentStateName, "Jump");
	if (resetDownwardVelocityForJumpStart && currentVelocity.y > 0.0f) {
		currentVelocity.y = 0.0f;
		velocityAdjusted = true;
	}

	if (velocityAdjusted) {
		this->setVelocity(currentVelocity);
	}

	const bool enteringAerialState =
		!strcmp(currentStateName, "Rising") ||
		!strcmp(currentStateName, "Jump") ||
		!strcmp(currentStateName, "Falling");
	if (!enteringAerialState) {
		_longJumpMomentumActive = false;
		_longJumpMomentumDirection = 0;
		_longJumpMomentumSpeed = 0.0f;
	}
	else {
		const char* previousStateName = previousState->getName();
		const bool launchedFromRunState =
			previousStateName &&
			(!strcmp(previousStateName, "RunningLeft") || !strcmp(previousStateName, "RunningRight"));
		const float horizontalLaunchSpeed = std::fabs(currentVelocity.x);
		const int launchDirection =
			(currentVelocity.x > kHorizontalVelocityEpsilon) ? 1 :
			((currentVelocity.x < -kHorizontalVelocityEpsilon) ? -1 : 0);
		if (launchedFromRunState &&
			_isRunRequested() &&
			launchDirection != 0 &&
			horizontalLaunchSpeed >= kLongJumpLaunchSpeedThreshold) {
			_longJumpMomentumActive = true;
			_longJumpMomentumDirection = launchDirection;
			_longJumpMomentumSpeed = horizontalLaunchSpeed;
		}
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
	_kinematic2DState().pendingTransitionFootCorrection = std::fabs(deltaY);
}
