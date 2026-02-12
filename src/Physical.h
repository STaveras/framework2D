#pragma once

// I'm going to just... plug in a library :|

#include "Positionable.h"
#include "Cyclable.h"

#include <unordered_map>
#include <unordered_set>

class GameObject;
struct CollisionContact;

// For actually making physics a thing in this "engine" sometime in the future...
class Physical : public Positionable, public Cyclable
{
public:
	struct KinematicConfig2D
	{
		bool enabled = false;
		vector2 upAxis = vector2(0.0f, -1.0f);
		float groundNormalThreshold = 0.2f;
		float wallNormalThreshold = 0.55f;
		float oneWayEpsilon = 0.5f;
		float supportProbeFootAboveTolerance = 2.0f;
		float supportProbeFootBelowTolerance = 20.0f;
		int minimumSupportSamples = 1;
		float maxStepHeight = 8.0f;
		float dropThroughDefaultDuration = 0.2f;
	};

	struct KinematicState2D
	{
		bool grounded = false;
		float timeWithoutGroundContact = 0.0f;
		float pendingTransitionFootCorrection = 0.0f;
		float dropThroughTimer = 0.0f;
		bool dropThroughResumePending = false;
		float dropThroughResumeTopY = 0.0f;
		bool dropThroughJumpWasDown = false;
		float fallingLandingDebounceTimer = 0.0f;

		float telemetryPendingWallCorrectionX = 0.0f;
		float telemetryLastWallCorrectionX = 0.0f;
		int telemetryPendingGroundContacts = 0;
		int telemetryPendingWallContacts = 0;
		int telemetryLastGroundContacts = 0;
		int telemetryLastWallContacts = 0;

		bool dropThroughRequested = false;
		float requestedDropThroughDuration = 0.0f;
		bool dropThroughResumeTopProvided = false;

		GameObject* groundObject = nullptr;
		std::unordered_set<GameObject*> groundContacts;
		std::unordered_map<GameObject*, int> groundContactFrameCount;
	};

protected:

	bool _static;

	float _mass;
	float _rotation;
	float _restitution;

	vector2 _velocity;
	KinematicConfig2D _kinematicConfig;
	KinematicState2D _kinematicState;

public:
	Physical(void) :
		Cyclable(),
		_static(false),
		_mass(1.0f),
		_rotation(0.0f),
		_restitution(1.0f), // bouncy ;)
		_velocity(0.0f, 0.0f) {

	}

	bool isStatic(void) const { return _static; }

	float getMass(void) const { return _mass; }
	float getRotation(void) const { return _rotation; }
	float getRestitution(void) const { return _restitution; }

	void setMass(float mass) { _mass = mass; }
	void setRotation(float rotation) { _rotation = rotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)
	void setStatic(bool isStatic) { _static = isStatic; }

	vector2 getVelocity(void) const { return _velocity; }
	void setVelocity(vector2 velocity) { _velocity = velocity; }

	void setKinematicConfig2D(const KinematicConfig2D& config) { _kinematicConfig = config; }
	const KinematicConfig2D& getKinematicConfig2D(void) const { return _kinematicConfig; }
	KinematicState2D& getKinematicState2D(void) { return _kinematicState; }
	const KinematicState2D& getKinematicState2D(void) const { return _kinematicState; }
	bool hasKinematic2D(void) const { return _kinematicConfig.enabled; }
	void enableKinematic2D(bool enabled = true) { _kinematicConfig.enabled = enabled; }

	void resetKinematicState2D(void);
	void requestDropThrough(float durationSeconds = 0.0f, float resumeTopY = 0.0f, bool hasResumeTopY = false);
	void beginKinematicFrame(float dt);
	void processKinematicContact(const CollisionContact& contact, int horizontalIntent = 0);
	void finalizeKinematicFrame(float dt);

	virtual void addImpulse(vector2 direction, double force) {

		_velocity += (direction * (float)force);

	}

	virtual void update(float time) {

		if (_static)
			return;

		this->setPosition(this->getPosition() += _velocity * time);
	}

	virtual void collision(Physical* body) {

		if (_static)
			return;

		vector2 normal = this->getPosition() - body->getPosition();
		normal.normalize();
		vector2 relativeVelocity = this->getVelocity() - body->getVelocity();
		float velAlongNormal = dot(relativeVelocity, normal);
		if (velAlongNormal > 0) {
			return;
		}
		float e = std::min(this->getRestitution(), body->getRestitution());
		float j = -(1 + e) * velAlongNormal;
		j /= 1 / this->getMass() + 1 / body->getMass();
		vector2 impulse = j * normal;

		//addImpulse(normal, j);
		//this->setVelocity(this->getVelocity() + impulse / this->getMass());
		//this->setPosition(this->getPosition() + normal * OVERLAP_ALLOWANCE);
		//body->setVelocity(body->getVelocity() - impulse / body->getMass());
		//body->setPosition(body->getPosition() - normal * OVERLAP_ALLOWANCE);
	}
};
