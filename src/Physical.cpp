#include "Physical.h"

#include "Collidable.h"
#include "GameObject.h"
#include "Kinematics2D.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kHorizontalSeparationEpsilon = 0.01f;
constexpr float kMaxHorizontalSeparationPerContact = 4.0f;
}

void Physical::resetKinematicState2D(void)
{
	_kinematicState.grounded = false;
	_kinematicState.timeWithoutGroundContact = 0.0f;
	_kinematicState.pendingTransitionFootCorrection = 0.0f;
	_kinematicState.dropThroughTimer = 0.0f;
	_kinematicState.dropThroughResumePending = false;
	_kinematicState.dropThroughResumeTopY = 0.0f;
	_kinematicState.dropThroughJumpWasDown = false;
	_kinematicState.fallingLandingDebounceTimer = 0.0f;

	_kinematicState.telemetryPendingWallCorrectionX = 0.0f;
	_kinematicState.telemetryLastWallCorrectionX = 0.0f;
	_kinematicState.telemetryPendingGroundContacts = 0;
	_kinematicState.telemetryPendingWallContacts = 0;
	_kinematicState.telemetryLastGroundContacts = 0;
	_kinematicState.telemetryLastWallContacts = 0;

	_kinematicState.dropThroughRequested = false;
	_kinematicState.requestedDropThroughDuration = 0.0f;
	_kinematicState.dropThroughResumeTopProvided = false;

	_kinematicState.groundObject = nullptr;
	_kinematicState.groundContacts.clear();
	_kinematicState.groundContactFrameCount.clear();
}

void Physical::requestDropThrough(float durationSeconds, float resumeTopY, bool hasResumeTopY)
{
	if (durationSeconds <= 0.0f) {
		durationSeconds = _kinematicConfig.dropThroughDefaultDuration;
	}

	_kinematicState.dropThroughRequested = true;
	_kinematicState.requestedDropThroughDuration = std::max(0.0f, durationSeconds);
	_kinematicState.dropThroughResumeTopY = resumeTopY;
	_kinematicState.dropThroughResumeTopProvided = hasResumeTopY;
}

void Physical::beginKinematicFrame(float dt)
{
	if (!_kinematicConfig.enabled) {
		return;
	}

	_kinematicState.telemetryLastWallCorrectionX = _kinematicState.telemetryPendingWallCorrectionX;
	_kinematicState.telemetryPendingWallCorrectionX = 0.0f;
	_kinematicState.telemetryLastGroundContacts = _kinematicState.telemetryPendingGroundContacts;
	_kinematicState.telemetryPendingGroundContacts = 0;
	_kinematicState.telemetryLastWallContacts = _kinematicState.telemetryPendingWallContacts;
	_kinematicState.telemetryPendingWallContacts = 0;

	if (_kinematicState.fallingLandingDebounceTimer > 0.0f) {
		_kinematicState.fallingLandingDebounceTimer = std::max(0.0f, _kinematicState.fallingLandingDebounceTimer - dt);
	}

	if (_kinematicState.dropThroughTimer > 0.0f) {
		_kinematicState.dropThroughTimer = std::max(0.0f, _kinematicState.dropThroughTimer - dt);
	}

	if (_kinematicState.dropThroughRequested) {
		_kinematicState.dropThroughTimer = std::max(_kinematicState.dropThroughTimer, _kinematicState.requestedDropThroughDuration);
		_kinematicState.dropThroughResumePending = _kinematicState.dropThroughResumeTopProvided;
		_kinematicState.groundContacts.clear();
		_kinematicState.groundContactFrameCount.clear();
		_kinematicState.groundObject = nullptr;
		_kinematicState.grounded = false;
		_kinematicState.timeWithoutGroundContact = 0.0f;
		_kinematicState.dropThroughRequested = false;
		_kinematicState.requestedDropThroughDuration = 0.0f;
		_kinematicState.dropThroughResumeTopProvided = false;
	}

	if (_kinematicState.dropThroughResumePending) {
		GameObject* object = dynamic_cast<GameObject*>(this);
		if (!object) {
			_kinematicState.dropThroughResumePending = false;
			_kinematicState.dropThroughResumeTopY = 0.0f;
		}
		else {
			Collidable* collidable = object->getCollidable();
			vector2 min(0.0f, 0.0f);
			vector2 max(0.0f, 0.0f);
			if (!collidable || !Kinematics2D::tryGetBounds(collidable, min, max) || min.y >= _kinematicState.dropThroughResumeTopY) {
				_kinematicState.dropThroughResumePending = false;
				_kinematicState.dropThroughResumeTopY = 0.0f;
			}
		}
	}
}

void Physical::processKinematicContact(const CollisionContact& contact, int horizontalIntent)
{
	if (!_kinematicConfig.enabled || !contact.other) {
		return;
	}

	GameObject* otherObject = contact.other;
	Collidable* otherCollidable = contact.otherCollidable;

	if (contact.phase == CollisionPhase::Exit) {
		_kinematicState.groundContacts.erase(otherObject);
		_kinematicState.groundContactFrameCount.erase(otherObject);
		return;
	}

	if (otherCollidable && otherCollidable->hasSurfaceFlag(SurfaceFlags::OneWay)) {
		if (_kinematicState.dropThroughTimer > 0.0f || _kinematicState.dropThroughResumePending) {
			_kinematicState.groundContacts.erase(otherObject);
			_kinematicState.groundContactFrameCount.erase(otherObject);
			return;
		}
	}

	const bool groundContact = Kinematics2D::isGroundContact(
		contact,
		_kinematicConfig.groundNormalThreshold,
		_kinematicConfig.supportProbeFootAboveTolerance,
		_kinematicConfig.supportProbeFootBelowTolerance);
	if (groundContact) {
		_kinematicState.groundContacts.insert(otherObject);
		_kinematicState.timeWithoutGroundContact = 0.0f;
		++_kinematicState.telemetryPendingGroundContacts;

		int& frames = _kinematicState.groundContactFrameCount[otherObject];
		++frames;
	}
	else {
		_kinematicState.groundContacts.erase(otherObject);
		_kinematicState.groundContactFrameCount.erase(otherObject);
	}

	if (Kinematics2D::isWallBlockingContact(contact, horizontalIntent, _kinematicConfig.wallNormalThreshold)) {
		++_kinematicState.telemetryPendingWallContacts;

		if (contact.normal.has_value()) {
			const vector2 normal = contact.normal.value();
			const float absNormalX = std::fabs(normal.x);
			float separationX = kHorizontalSeparationEpsilon;
			if (contact.penetrationDepth.has_value() && contact.penetrationDepth.value() > 0.0f) {
				separationX += contact.penetrationDepth.value() / std::max(absNormalX, 0.001f);
			}
			separationX = std::min(separationX, kMaxHorizontalSeparationPerContact);
			const float correctionX = -std::copysign(separationX, normal.x);
			this->setPosition(this->getPosition().x + correctionX, this->getPosition().y);
			_kinematicState.telemetryPendingWallCorrectionX += correctionX;
		}
	}
}

void Physical::finalizeKinematicFrame(float dt)
{
	if (!_kinematicConfig.enabled) {
		return;
	}

	if (_kinematicState.groundContacts.empty()) {
		_kinematicState.grounded = false;
		_kinematicState.groundObject = nullptr;
		_kinematicState.timeWithoutGroundContact += std::max(0.0f, dt);
		return;
	}

	_kinematicState.grounded = true;
	_kinematicState.timeWithoutGroundContact = 0.0f;
	_kinematicState.groundObject = *_kinematicState.groundContacts.begin();
}
