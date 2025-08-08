// Collidable.h
#pragma once

#include "COL_OBJ_TYPE.h"

#ifndef _COLLIDABLE_H_
#define _COLLIDABLE_H_

#include "Positionable.h"

class Collidable : public Positionable
{
	Collidable(void) :Positionable(), m_eType(COL_OBJ_VOID) {}

private:
	COL_OBJ_TYPE m_eType;
	bool _isActive = true; // Collidable objects are active by default

protected:
	explicit Collidable(COL_OBJ_TYPE type) : Positionable(), m_eType(type) {}

public:
	// Copy constructor
	Collidable(const Collidable& other)
		: Positionable(other.getPosition()), m_eType(other.m_eType), _isActive(other._isActive) {}

	COL_OBJ_TYPE getType(void) const { return m_eType; }

	bool isActive(void) const { return _isActive; }
	void setActive(bool isActive) { _isActive = isActive; }

	virtual bool collidesWith(vector2 point) { return _position == point; } // issa joke
	virtual bool collidesWith(const Collidable* collidable) { return this->collidesWith(collidable->getPosition()); }

	virtual ~Collidable(void) = default;
};

#endif