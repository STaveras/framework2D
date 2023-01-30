#pragma once

// I'm going to just... plug in a library :|

#include "Positionable.h"
#include "Cyclable.h"

const float OVERLAP_ALLOWANCE = 1.0;

// For actually making physics a thing in this "engine" sometime in the future...
class Physical : public Positionable, public Cyclable
{
	//using Cyclable::start;
	//using Cyclable::finish;

protected:
	float _mass;
	float _rotation;
	float _restitution;

	vector2 _velocity;

public:
	Physical(void) :
		Cyclable(),
		_mass(0.0f),
		_rotation(0.0f),
		_restitution(1.0f), // bouncy ;)
		_velocity(0.0f, 0.0f) {

	}

	float getMass(void) const { return _mass; }
	float getRotation(void) const { return _rotation; }
	float getRestitution(void) const { return _restitution; }

	void setMass(float mass) { _mass = mass; }
	void setRotation(float rotation) { _rotation = rotation; } // UNDONE: PLEASE DON'T USE THIS ON ANYTHING OTHER THAN A CAMERA (cus collision objects don't rotate yet... :/)

	vector2 getVelocity(void) const { return _velocity; }
	void setVelocity(vector2 velocity) { _velocity = velocity; }

	virtual void addImpulse(vector2 direction, double force) {

		_velocity += (direction * (float)force);

	}

	virtual void update(float time) {

		this->setPosition(this->getPosition() += _velocity * time);

	}

	virtual void collision(Physical* body) {

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
		//this->setVelocity(this->getVelocity() + impulse / this->getMass());
		//this->setPosition(this->getPosition() + normal * OVERLAP_ALLOWANCE);
		//body->setVelocity(body->getVelocity() - impulse / body->getMass());
		//body->setPosition(body->getPosition() - normal * OVERLAP_ALLOWANCE);
	}
};
