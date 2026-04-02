// CharacterUpdate.cpp

#include "Character.h"
#include "CharacterTuning.h"

#include "../Animation.h"
#include "../CollidableGroup.h"
#include "../GameState.h"
#include "../Telemetry2D.h"
#include "../Kinematics2D.h"
#include "../Polygon.h"
#include "../Square.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
enum class AutoGroundShape {
	None,
	Square,
	Polygon,
	Other
};

using AutoTestRuntime = Telemetry2D::Runtime;

AutoGroundShape classifyAutoGroundShape(const Tile* tile)
{
	if (!tile) {
		return AutoGroundShape::None;
	}

	Collidable* collidable = ((Tile*)tile)->getCollidable();
	if (!collidable || !collidable->isActive()) {
		return AutoGroundShape::Other;
	}

	if (Kinematics2D::isSquareOnly(collidable)) {
		return AutoGroundShape::Square;
	}

	if (Kinematics2D::hasPolygonSurface(collidable)) {
		return AutoGroundShape::Polygon;
	}

	return AutoGroundShape::Other;
}

const char* toAutoGroundShapeString(AutoGroundShape shape)
{
	switch (shape) {
	case AutoGroundShape::None:
		return "none";
	case AutoGroundShape::Square:
		return "square";
	case AutoGroundShape::Polygon:
		return "polygon";
	default:
		return "other";
	}
}

const char* toAutoSupportSourceString(AutoSupportSource source)
{
	switch (source) {
	case AutoSupportSource::Left:
		return "left";
	case AutoSupportSource::Center:
		return "center";
	case AutoSupportSource::Right:
		return "right";
	case AutoSupportSource::UphillProbe:
		return "uphill_probe";
	case AutoSupportSource::Sticky:
		return "sticky";
	default:
		return "none";
	}
}

void initializeAutoTestRuntime(AutoTestRuntime& runtime)
{
	Telemetry2D::initialize(runtime, kAutoDefaultTelemetryPath);
}
} // namespace

namespace CharacterPrivate
{
Telemetry2D::Runtime& AutoRuntime()
{
	static Telemetry2D::Runtime runtime;
	return runtime;
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
            if (tile->getLayerCollisionMode() != TileCollisionMode::None && contact.phase == CollisionPhase::Enter) {
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
	            if (GameObjectState* state = this->getState()) {
	                if (!strcmp(state->getName(), "Falling") && !_canTriggerGroundCollisionFromFalling()) {
	                    return NULL;
	                }
	            }
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
    } else {
        vector2 directionToObject = tile->getPosition() - this->getPosition();
        directionToObject.normalize();
        if (directionToObject.y < -0.5f) {
            return "JUMP_RELEASED";
        }
	        if (directionToObject.y > kGroundNormalThreshold) {
	            if (GameObjectState* state = this->getState()) {
	                if (!strcmp(state->getName(), "Falling") && !_canTriggerGroundCollisionFromFalling()) {
	                    return NULL;
	                }
	            }
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
		_kinematic2DState().groundContacts.erase(tile);
		_refreshGroundTile();
		return;
	}

	const bool isGroundContact = _isGroundContact(contact);
	if (isGroundContact) {
		_kinematic2DState().groundContacts.insert(tile);
		_kinematic2DState().timeWithoutGroundContact = 0.0f;
		_refreshGroundTile();
	}
	else {
		_kinematic2DState().groundContacts.erase(tile);
		_refreshGroundTile();
	}

	if (isGroundContact && contact.phase != CollisionPhase::Exit) {
		++_kinematic2DState().telemetryPendingGroundContacts;
	}

	if (contact.overlapping &&
		tile->getTileType() == "tile" &&
		!tile->isNonCollidingLayer() &&
		(!_isOneWayTile(tile) || _canCollideWithOneWayTile(tile)) &&
		contact.normal.has_value()) {
		const int horizontalIntent = _getHorizontalInput();
		if (horizontalIntent != 0) {
			float maxStepUpDistance = kDefaultMaxSnapPerFrame;
			Tile* stepContextTile = tile ? tile : _tile;
			if (stepContextTile && stepContextTile->getTileSet()) {
				maxStepUpDistance = std::max(1.0f, stepContextTile->getTileSet()->getTileSize() * 0.5f * kUpwardSnapMultiplier);
			}
			maxStepUpDistance = std::min(maxStepUpDistance, kMaxAutoStepUpDistance);

			float footY = this->getPosition().y;
			if (Collidable* bodyCollidable = this->getCollidable()) {
				if (bodyCollidable->getType() == COL_OBJ_SQUARE) {
					Square* bodySquare = (Square*)bodyCollidable;
					footY = bodySquare->getMax().y;
				}
			}

			if (_isWallBlockingContact(contact, horizontalIntent, footY, maxStepUpDistance)) {
				++_kinematic2DState().telemetryPendingWallContacts;

				bool isGroundedForStepAssist = false;
				if (_kinematic2DState().timeWithoutGroundContact < kGroundLossGraceSeconds) {
					if (GameObjectState* currentState = this->getState()) {
						const char* stateName = currentState->getName();
						isGroundedForStepAssist =
							_isGroundedLocomotionState(stateName) ||
							!strcmp(stateName, "Attack01") ||
							!strcmp(stateName, "Attack02");
					}
				}

				if (isGroundedForStepAssist) {
					float stepSupportY = footY;
					Tile* stepSupportTile = _findGroundSupportTile(footY, maxStepUpDistance, stepSupportY);
					const float stepDeltaY = stepSupportY - footY;
					if (stepSupportTile && stepDeltaY < -kStepUpAssistEpsilon) {
						this->setPosition(
							this->getPosition().x,
							this->getPosition().y + std::max(-maxStepUpDistance, stepDeltaY));
						return;
					}
				}

				const vector2 normal = contact.normal.value();
				const float absNormalX = std::fabs(normal.x);
				float separationX = kHorizontalSeparationEpsilon;
				if (contact.penetrationDepth.has_value() && contact.penetrationDepth.value() > 0.0f) {
					separationX += contact.penetrationDepth.value() / std::max(absNormalX, 0.001f);
				}
				separationX = std::min(separationX, kMaxHorizontalSeparationPerContact);

				const float correctionX = -std::copysign(separationX, normal.x);
				this->setPosition(this->getPosition().x + correctionX, this->getPosition().y);
				_kinematic2DState().telemetryPendingWallCorrectionX += correctionX;
			}
		}
	}

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision && Debug::dbgTiles)
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

void Character::update(float time)
{
	AutoTestRuntime& autoRuntime = CharacterPrivate::AutoRuntime();
	initializeAutoTestRuntime(autoRuntime);
	autoRuntime.elapsedSeconds += std::max(0.0, (double)time);
	this->beginKinematicFrame(time);

	_refreshGroundTile();

	if (_isDropThroughRequested()) {
		bool groundedOnOneWay = _isOneWayTile(_tile);

		if (!groundedOnOneWay) {
			for (GameObject* contactObject : _kinematic2DState().groundContacts) {
				Tile* contactTile = dynamic_cast<Tile*>(contactObject);
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

	if (GameObjectState* preUpdateState = this->getState()) {
		const char* preUpdateStateName = preUpdateState->getName();
		const bool preUpdateStateHasCollisionShape = preUpdateState->getCollidable() != NULL;
		const bool freezeHorizontalPreUpdate =
			!strcmp(preUpdateStateName, "Attack01") ||
			!strcmp(preUpdateStateName, "Attack02");
		if (freezeHorizontalPreUpdate || !preUpdateStateHasCollisionShape) {
			vector2 preUpdateVelocity = this->getVelocity();
			const bool shouldFreezeVertical = !preUpdateStateHasCollisionShape;
			if (std::fabs(preUpdateVelocity.x) > kHorizontalVelocityEpsilon ||
				(shouldFreezeVertical && std::fabs(preUpdateVelocity.y) > kHorizontalVelocityEpsilon)) {
				preUpdateVelocity.x = 0.0f;
				if (shouldFreezeVertical) {
					preUpdateVelocity.y = 0.0f;
				}
				this->setVelocity(preUpdateVelocity);
			}
		}
	}

	GameObject::update(time);
    // Proactive ground sampling before relying on collision contacts. This
    // helps start levels grounded even if no collision Event has fired yet.
    // Skip this sampling for aerial states (jumping or falling) to avoid
    // snapping the character back down when they are in the air.  It is
    // performed only when the previous state is not an aerial state.
    {
        bool doPreSupportSample = true;
        if (GameObjectState* preState = this->getState()) {
            const char* name = preState->getName();
            // Identify aerial states where we do not want to snap to ground
            if (!std::strcmp(name, "Rising") || !std::strcmp(name, "Jump") || !std::strcmp(name, "Falling")) {
                doPreSupportSample = false;
            }
        }
        if (doPreSupportSample) {
            float baseSnapPerFrame = kDefaultMaxSnapPerFrame;
            if (_tile && _tile->getTileSet()) {
                baseSnapPerFrame = std::max(1.0f, _tile->getTileSet()->getTileSize() * 0.5f);
            }
            float maxSnapPerFrame = baseSnapPerFrame;
            if (_kinematic2DState().pendingTransitionFootCorrection > 0.0f) {
                maxSnapPerFrame = std::max(baseSnapPerFrame, _kinematic2DState().pendingTransitionFootCorrection + 1.0f);
            }
            const float horizontalTravelPerFrame = std::fabs(this->getVelocity().x) * time;
            maxSnapPerFrame = std::max(maxSnapPerFrame, horizontalTravelPerFrame + kHorizontalSnapTravelPadding);
	            const float maxUpwardSnapPerFrame = std::min(
	                kMaxAutoStepUpDistance,
	                std::max(maxSnapPerFrame, baseSnapPerFrame * kUpwardSnapMultiplier));

            float footY = this->getPosition().y;
            if (Collidable* bodyCollidable = this->getCollidable()) {
                if (bodyCollidable->getType() == COL_OBJ_SQUARE) {
                    Square* bodySquare = (Square*)bodyCollidable;
                    footY = bodySquare->getMax().y;
                }
            }

            float preSupportY = footY;
            Tile* preSupportTile = _findGroundSupportTile(footY, maxSnapPerFrame, preSupportY);
            if (preSupportTile) {
                // Establish support before processing inputs, so we don't float
                // for a frame at scene start or after teleports.
                _tile = preSupportTile;
                const float targetFootY = preSupportY;
                float deltaY = targetFootY - footY;
                if (std::fabs(deltaY) > 0.001f) {
                    if (deltaY < 0.0f) {
                        deltaY = std::max(-maxUpwardSnapPerFrame, deltaY);
                    }
                    else {
                        deltaY = std::min(maxSnapPerFrame, deltaY);
                    }
                    this->setPosition(this->getPosition().x, this->getPosition().y + deltaY);
                }
                _kinematic2DState().timeWithoutGroundContact = 0.0f;
            }
        }
    }

	float baseSnapPerFrame = kDefaultMaxSnapPerFrame;
	if (_tile && _tile->getTileSet()) {
		baseSnapPerFrame = std::max(1.0f, _tile->getTileSet()->getTileSize() * 0.5f);
	}
	float maxSnapPerFrame = baseSnapPerFrame;
	if (_kinematic2DState().pendingTransitionFootCorrection > 0.0f) {
		maxSnapPerFrame = std::max(baseSnapPerFrame, _kinematic2DState().pendingTransitionFootCorrection + 1.0f);
	}
	const float horizontalTravelPerFrame = std::fabs(this->getVelocity().x) * time;
	maxSnapPerFrame = std::max(maxSnapPerFrame, horizontalTravelPerFrame + kHorizontalSnapTravelPadding);
	const float maxUpwardSnapPerFrame = std::min(
		kMaxAutoStepUpDistance,
		std::max(maxSnapPerFrame, baseSnapPerFrame * kUpwardSnapMultiplier));

	float footY = this->getPosition().y;
	if (Collidable* bodyCollidable = this->getCollidable()) {
		if (bodyCollidable->getType() == COL_OBJ_SQUARE) {
			Square* bodySquare = (Square*)bodyCollidable;
			footY = bodySquare->getMax().y;
		}
	}

	int supportSampleSource = (int)AutoSupportSource::None;
	float supportY = footY;
	Tile* supportTile = _findGroundSupportTile(footY, maxSnapPerFrame, supportY, &supportSampleSource);
	if (supportTile && _tile && supportTile != _tile) {
		float stickySupportY = footY;
		if (_findSupportOnTile(_tile, footY, maxSnapPerFrame, stickySupportY)) {
			const float switchDelta = supportY - stickySupportY;
			const float hysteresis =
				(switchDelta < 0.0f) ?
				kSupportSwitchHysteresisUp :
				kSupportSwitchHysteresisDown;
			if (std::fabs(switchDelta) <= hysteresis) {
				supportTile = _tile;
				supportY = stickySupportY;
				supportSampleSource = (int)AutoSupportSource::Sticky;
			}
		}
	}

	bool hasGroundSupport = (supportTile != NULL);
	GameObjectState* state = this->getState();
	if (state && !strcmp(state->getName(), "Falling") && !_canTriggerGroundCollisionFromFalling()) {
		// Ignore transient support right after entering Falling to prevent
		// edge-corner collider jitter from pinning the character to the ledge.
		hasGroundSupport = false;
		supportTile = NULL;
		supportSampleSource = (int)AutoSupportSource::None;
	}
	if (supportTile) {
		_tile = supportTile;
	}

	if (!hasGroundSupport) {
		_kinematic2DState().timeWithoutGroundContact += time;
	}
	else {
		_kinematic2DState().timeWithoutGroundContact = 0.0f;
	}

	if (state) {
		const char* stateName = state->getName();
		if (hasGroundSupport && !strcmp(stateName, "Falling") && _canTriggerGroundCollisionFromFalling()) {
			this->sendInput("GROUND_COLLISION");
			state = this->getState();
			if (!state) {
				_kinematic2DState().pendingTransitionFootCorrection = 0.0f;
				return;
			}
			stateName = state->getName();
		}

		const bool groundedLocomotionState = _isGroundedLocomotionState(stateName);
		const bool shouldApplyGroundSnap =
			groundedLocomotionState ||
			!strcmp(stateName, "Attack01") ||
			!strcmp(stateName, "Attack02");

		if (hasGroundSupport && shouldApplyGroundSnap) {
			float targetFootY = supportY;
			if (supportTile) {
				if (Collidable* supportCollidable = supportTile->getCollidable()) {
					if (!Kinematics2D::isSquareOnly(supportCollidable)) {
						// Keep the character slightly above sloped/polygon supports to avoid visible embedding.
						targetFootY -= kSlopeFootClearance;
					}
				}
			}

			float deltaY = targetFootY - footY;
			if (std::fabs(deltaY) > 0.001f) {
				if (deltaY < 0.0f) {
					deltaY = std::max(-maxUpwardSnapPerFrame, deltaY);
				}
				else {
					deltaY = std::min(maxSnapPerFrame, deltaY);
				}
				this->setPosition(this->getPosition().x, this->getPosition().y + deltaY);
				footY += deltaY;
			}
		}

		const bool canInputMove =
			!strcmp(stateName, "RunningLeft") ||
			!strcmp(stateName, "RunningRight") ||
			!strcmp(stateName, "Falling") ||
			!strcmp(stateName, "Jump") ||
			!strcmp(stateName, "Rising");
		const bool canResidualMove =
			canInputMove ||
			!strcmp(stateName, "Idle") ||
			!strcmp(stateName, "Landing");
		const bool isAerialState =
			!strcmp(stateName, "Falling") ||
			!strcmp(stateName, "Jump") ||
			!strcmp(stateName, "Rising");
			
		const bool groundedForMovement = hasGroundSupport || (_kinematic2DState().timeWithoutGroundContact < kGroundLossGraceSeconds);

		const int horizontalInput = canInputMove ? _getHorizontalInput() : 0;
		const bool hasDirectionalIntent = horizontalInput != 0;
		const bool runRequested = canInputMove && groundedForMovement && _isRunRequested();
		_runBoostActive = runRequested && hasDirectionalIntent && _stamina > 0.0f;

		if (_runBoostActive) {
			_stamina = std::max(0.0f, _stamina - (kStaminaDrainPerSecond * time));
		}
		else {
			_stamina = std::min(_maxStamina, _stamina + (kStaminaRegenPerSecond * time));
		}

		if (_stamina <= 0.0f) {
			_runBoostActive = false;
		}

		Player* player = Engine2D::getGame()->getPlayerWith(this);
		bool downHeld = false;
		if (player && player->getController()) {
			if (Action* downAction = player->getController()->getAction("DOWN")) {
				downHeld = downAction->isActive();
			}
		}

		float horizontalVelocity = this->getVelocity().x;
		const float startingAbsHorizontalSpeed = std::fabs(horizontalVelocity);
		if (canResidualMove) {
			const float groundedMaxSpeed = _runBoostActive ? kRunMaxHorizontalSpeed : kWalkMaxHorizontalSpeed;
			const float targetVelocityX = hasDirectionalIntent ?
				((float)horizontalInput * (groundedForMovement ? groundedMaxSpeed : kAirMaxHorizontalSpeed)) :
				0.0f;
			const bool reversingInput =
				hasDirectionalIntent && ((horizontalVelocity * (float)horizontalInput) < -kHorizontalVelocityEpsilon);

			float acceleration = 0.0f;
			if (hasDirectionalIntent) {
				if (groundedForMovement) {
					acceleration = reversingInput ?
						kGroundTurnAcceleration :
						(_runBoostActive ? kRunGroundAcceleration : kWalkGroundAcceleration);
				}
				else {
					acceleration = reversingInput ? kAirTurnAcceleration : kAirAcceleration;
				}
			}
			else {
				acceleration = groundedForMovement ? kGroundDeceleration : kAirDeceleration;
			}

			const float maxDelta = acceleration * time;
			if (horizontalVelocity < targetVelocityX) {
				horizontalVelocity = std::min(horizontalVelocity + maxDelta, targetVelocityX);
			}
			else if (horizontalVelocity > targetVelocityX) {
				horizontalVelocity = std::max(horizontalVelocity - maxDelta, targetVelocityX);
			}

			if (groundedForMovement) {
				horizontalVelocity = std::max(-groundedMaxSpeed, std::min(groundedMaxSpeed, horizontalVelocity));
			}
			else {
				const float airSpeedClamp = std::max(kAirMaxHorizontalSpeed, startingAbsHorizontalSpeed);
				horizontalVelocity = std::max(-airSpeedClamp, std::min(airSpeedClamp, horizontalVelocity));
			}

			if (!hasDirectionalIntent && std::fabs(horizontalVelocity) < kHorizontalVelocityEpsilon) {
				horizontalVelocity = 0.0f;
			}
		}
		else {
			horizontalVelocity = 0.0f;
			_runBoostActive = false;
		}

		if (!groundedForMovement && isAerialState && _longJumpMomentumActive) {
			const bool oppositeDirectionInput =
				hasDirectionalIntent && (horizontalInput != _longJumpMomentumDirection);
			const bool wrongDirectionVelocity =
				(horizontalVelocity * (float)_longJumpMomentumDirection) < -kHorizontalVelocityEpsilon;

			if (oppositeDirectionInput || wrongDirectionVelocity || _longJumpMomentumDirection == 0) {
				_longJumpMomentumActive = false;
				_longJumpMomentumDirection = 0;
				_longJumpMomentumSpeed = 0.0f;
			}
			else {
				_longJumpMomentumSpeed = std::max(0.0f, _longJumpMomentumSpeed - (kLongJumpMomentumDecayPerSecond * time));
				const float signedMomentumSpeed = (float)_longJumpMomentumDirection * _longJumpMomentumSpeed;
				const float signedHorizontalSpeed = horizontalVelocity * (float)_longJumpMomentumDirection;
				if (signedHorizontalSpeed < _longJumpMomentumSpeed) {
					horizontalVelocity = signedMomentumSpeed;
				}

				if (_longJumpMomentumSpeed <= kWalkMaxHorizontalSpeed) {
					_longJumpMomentumActive = false;
					_longJumpMomentumDirection = 0;
					_longJumpMomentumSpeed = 0.0f;
				}
			}
		}
		else if (groundedForMovement && _longJumpMomentumActive) {
			_longJumpMomentumActive = false;
			_longJumpMomentumDirection = 0;
			_longJumpMomentumSpeed = 0.0f;
		}

		vector2 velocity = this->getVelocity();
		velocity.x = horizontalVelocity;
		this->setVelocity(velocity);

		const bool lockDownwardVelocityOnGround =
			hasGroundSupport &&
			(groundedLocomotionState ||
			 !strcmp(stateName, "Attack01") ||
			 !strcmp(stateName, "Attack02") ||
			 !strcmp(stateName, "Dead"));
        if (lockDownwardVelocityOnGround && velocity.y > 0.0f) {
            velocity.y = 0.0f;
            this->setVelocity(velocity);
        }

        // Apply gravity when the character is airborne.  Without this the hero
        // retains whatever vertical velocity was applied at jump start and
        // never accelerates downward, which prevents normal jumping and
        // falling behaviour.  Only apply gravity when we no longer have
        // ground support (with grace) and are in an aerial state (Rising,
        // Jump, Falling).
        if (!groundedForMovement && isAerialState) {
            vector2 v = this->getVelocity();
            // Gravity acceleration increases the downward velocity (positive Y)
            v.y = std::min(kNormalFallMaxSpeed, v.y + (kGravityAcceleration * time));
            this->setVelocity(v);
        }

        // Apply fast fall acceleration when holding down while airborne and
        // already falling downward.  This builds upon the gravity update above.
        if (!groundedForMovement && isAerialState && downHeld && this->getVelocity().y > 0.0f) {
            vector2 v = this->getVelocity();
            v.y = std::min(kFastFallMaxSpeed, v.y + (kFastFallAcceleration * time));
            this->setVelocity(v);
        }

		if (canInputMove && hasDirectionalIntent && this->getRenderable()) {
			vector2 scale = this->getRenderable()->getScale();
			const float absScaleX = std::fabs(scale.x);
			this->getRenderable()->setScale((horizontalInput > 0) ? absScaleX : -absScaleX, scale.y);
		}

		if ((!strcmp(stateName, "RunningLeft") || !strcmp(stateName, "RunningRight")) &&
			state->getRenderable() &&
			state->getRenderable()->getRenderableType() == RENDERABLE_TYPE_ANIMATION) {
			Animation* runAnimation = (Animation*)state->getRenderable();
			const float targetAnimationSpeed = _runBoostActive ? kRunBoostAnimationSpeed : kRunAnimationSpeed;
			if (std::fabs(runAnimation->getSpeed() - targetAnimationSpeed) > 0.001f) {
				runAnimation->setSpeed(targetAnimationSpeed);
			}
		}

		if (player) {
//#if _DEBUG
			if (KEYBOARD) {
				if (Engine2D::getInput()->getKeyboard()->keyPressed(KEYBOARD->getKeys().KBK_F)) {
					this->sendInput("DEATH");
				}
			}
//#endif
			// Having this idea about conditional state changes,
			// like having a "KEEP_ALIVE" condition in the event queue, and in the lack of that condition,
			// the character will fall or otherwise change state

			// Grounded state is tracked by collision Enter/Stay/Exit callbacks.
			// Debounce loss slightly to avoid one-frame Enter/Exit jitter.
			if (!hasGroundSupport && _kinematic2DState().timeWithoutGroundContact >= kGroundLossGraceSeconds) {
				if (_isGroundedLocomotionState(stateName)) {
					this->sendInput("IN_AIR");
				}
			}
		} // if (Player)
	}
	_kinematic2DState().pendingTransitionFootCorrection = 0.0f;
	if (Telemetry2D::isEnabled(autoRuntime)) {
		const vector2 pos = this->getPosition();
		const vector2 vel = this->getVelocity();
		const uint64_t replayTick = Engine2D::getSimulationTick();
		const double simulationElapsed = Engine2D::getSimulationElapsedSeconds();
		const double sampleTime = (simulationElapsed > 0.0) ? simulationElapsed : autoRuntime.elapsedSeconds;
		double sampleDt = std::max(0.0, (double)time);
		double dxSigned = 0.0;
		double dxAbs = 0.0;
		double dxSpeed = std::fabs((double)vel.x);
		double pathSpeed = std::sqrt(((double)vel.x * (double)vel.x) + ((double)vel.y * (double)vel.y));
		const int intent = _getHorizontalInput();

		if (autoRuntime.hasPreviousTelemetry) {
			const double observedDt = sampleTime - autoRuntime.previousTelemetryTime;
			if (observedDt > 0.000001) {
				const double dx = (double)pos.x - (double)autoRuntime.previousTelemetryPosition.x;
				const double dy = (double)pos.y - (double)autoRuntime.previousTelemetryPosition.y;
				dxSigned = dx;
				dxAbs = std::fabs(dx);
				dxSpeed = dxAbs / observedDt;
				pathSpeed = std::sqrt((dx * dx) + (dy * dy)) / observedDt;
				sampleDt = observedDt;
			}
		}
		else {
			dxSigned = (double)vel.x * sampleDt;
			dxAbs = std::fabs(dxSigned);
		}
		autoRuntime.netDxAccum += dxSigned;

		const bool rightHeld = intent > 0;

		const AutoGroundShape groundShape = classifyAutoGroundShape(_tile);
		if (autoRuntime.previousGrounded && !hasGroundSupport && rightHeld) {
			++autoRuntime.groundDropouts;
		}
		autoRuntime.previousGrounded = hasGroundSupport;

		const AutoSupportSource supportSource = hasGroundSupport ?
			(AutoSupportSource)supportSampleSource :
			AutoSupportSource::None;
		const char* contactClass = "unknown";
		if (_kinematic2DState().telemetryLastGroundContacts > _kinematic2DState().telemetryLastWallContacts) {
			contactClass = "ground";
		}
		else if (_kinematic2DState().telemetryLastWallContacts > _kinematic2DState().telemetryLastGroundContacts) {
			contactClass = "wall";
		}

		const double horizontalJitter = std::fabs(std::fabs((double)vel.x) - dxSpeed);
		autoRuntime.maxHorizontalJitter = std::max(autoRuntime.maxHorizontalJitter, horizontalJitter);

		if (hasGroundSupport && rightHeld) {
			if (groundShape == AutoGroundShape::Square) {
				autoRuntime.squareDxSpeedSum += dxSpeed;
				autoRuntime.squarePathSpeedSum += pathSpeed;
				++autoRuntime.squareSampleCount;
			}
			else if (groundShape == AutoGroundShape::Polygon) {
				autoRuntime.polygonDxSpeedSum += dxSpeed;
				autoRuntime.polygonPathSpeedSum += pathSpeed;
				++autoRuntime.polygonSampleCount;
			}
		}

		const double squareDxAvg = (autoRuntime.squareSampleCount > 0) ?
			(autoRuntime.squareDxSpeedSum / (double)autoRuntime.squareSampleCount) :
			0.0;
		const double polygonDxAvg = (autoRuntime.polygonSampleCount > 0) ?
			(autoRuntime.polygonDxSpeedSum / (double)autoRuntime.polygonSampleCount) :
			0.0;
		const double squarePathAvg = (autoRuntime.squareSampleCount > 0) ?
			(autoRuntime.squarePathSpeedSum / (double)autoRuntime.squareSampleCount) :
			0.0;
		const double polygonPathAvg = (autoRuntime.polygonSampleCount > 0) ?
			(autoRuntime.polygonPathSpeedSum / (double)autoRuntime.polygonSampleCount) :
			0.0;

		Telemetry2D::Sample sample;
		sample.time = sampleTime;
		sample.dt = sampleDt;
		sample.replayTick = replayTick;
		sample.stateName = this->getState() ? this->getState()->getName() : "(null)";
		sample.position = pos;
		sample.velocity = vel;
		sample.intent = intent;
		sample.dxSigned = dxSigned;
		sample.dxAbs = dxAbs;
		sample.netDxAccum = autoRuntime.netDxAccum;
		sample.dxSpeed = dxSpeed;
		sample.pathSpeed = pathSpeed;
		sample.wallCorrectionX = _kinematic2DState().telemetryLastWallCorrectionX;
		sample.supportSource = toAutoSupportSourceString(supportSource);
		sample.contactClass = contactClass;
		sample.groundTile = _tile ? _tile->getTileIndex() : -1;
		sample.groundShape = toAutoGroundShapeString(groundShape);
		sample.grounded = hasGroundSupport ? 1 : 0;
		sample.contacts = _kinematic2DState().groundContacts.size();
		sample.dropouts = autoRuntime.groundDropouts;
		sample.jitter = horizontalJitter;
		sample.squareDxAvg = squareDxAvg;
		sample.polygonDxAvg = polygonDxAvg;
		sample.squarePathAvg = squarePathAvg;
		sample.polygonPathAvg = polygonPathAvg;
		Telemetry2D::appendSample(autoRuntime, sample);

		autoRuntime.previousTelemetryPosition = pos;
		autoRuntime.previousTelemetryTime = sampleTime;
		autoRuntime.hasPreviousTelemetry = true;
	}

#if _DEBUG
	if (DEBUGGING && Debug::dbgCollision) {
		static float collisionTelemetryTimer = 0.0f;
		collisionTelemetryTimer += std::max(0.0f, time);
		if (collisionTelemetryTimer >= 0.25f) {
			collisionTelemetryTimer = 0.0f;
			const vector2 pos = this->getPosition();
			const vector2 vel = this->getVelocity();
			const Tile* groundTile = _tile;
			const int groundTileIndex = groundTile ? groundTile->getTileIndex() : -1;
			const std::string groundLayerName =
				(groundTile && !groundTile->getLayerName().empty()) ? groundTile->getLayerName() : "(none)";
			char buffer[320];
			sprintf_s(
				buffer,
				sizeof(buffer),
				"Hero dbg: pos={%.2f,%.2f} vel={%.2f,%.2f} state=%s support=%s contacts=%zu groundTile=%d@%s noGroundT=%.3f dropT=%.3f\n",
				pos.x,
				pos.y,
				vel.x,
				vel.y,
				this->getState() ? this->getState()->getName() : "(null)",
				(_tile != NULL) ? "yes" : "no",
				_kinematic2DState().groundContacts.size(),
				groundTileIndex,
				groundLayerName.c_str(),
				_kinematic2DState().timeWithoutGroundContact,
				_kinematic2DState().dropThroughTimer);
			DEBUG_MSG(buffer);
		}
	}
#endif
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
