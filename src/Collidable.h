// Collidable.h
#pragma once

#include "COL_OBJ_TYPE.h"

#ifndef _COLLIDABLE_H_
#define _COLLIDABLE_H_

#include "Positionable.h"
#include "Types.h"

#include <cstdint>

enum class SurfaceFlags : uint32_t
{
	None = 0,
	Solid = 1u << 0,
	Walkable = 1u << 1,
	OneWay = 1u << 2,
	StepCandidate = 1u << 3
};

inline SurfaceFlags operator|(SurfaceFlags lhs, SurfaceFlags rhs)
{
	return (SurfaceFlags)((uint32_t)lhs | (uint32_t)rhs);
}

inline SurfaceFlags operator&(SurfaceFlags lhs, SurfaceFlags rhs)
{
	return (SurfaceFlags)((uint32_t)lhs & (uint32_t)rhs);
}

inline SurfaceFlags& operator|=(SurfaceFlags& lhs, SurfaceFlags rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

struct SurfaceTraits2D
{
	SurfaceFlags flags = SurfaceFlags::None;
	vector2 upNormal = vector2(0.0f, -1.0f);
	float oneWayEpsilon = 0.5f;
	float maxStepHeight = 8.0f;
};

class Collidable : public Positionable
{
	Collidable(void) :Positionable(), m_eType(COL_OBJ_VOID) {}

private:
	COL_OBJ_TYPE m_eType;
	bool _isActive = true; // Collidable objects are active by default
	SurfaceTraits2D _surfaceTraits;

protected:
	explicit Collidable(COL_OBJ_TYPE type) : Positionable(), m_eType(type) {}

public:
	// Copy constructor
	Collidable(const Collidable& other)
		: Positionable(other.getPosition()), m_eType(other.m_eType), _isActive(other._isActive), _surfaceTraits(other._surfaceTraits) {}

	COL_OBJ_TYPE getType(void) const { return m_eType; }

	bool isActive(void) const { return _isActive; }
	void setActive(bool isActive) { _isActive = isActive; }

	void setSurfaceTraits(const SurfaceTraits2D& traits) { _surfaceTraits = traits; }
	const SurfaceTraits2D& getSurfaceTraits(void) const { return _surfaceTraits; }
	bool hasSurfaceFlag(SurfaceFlags flag) const
	{
		return (((uint32_t)(_surfaceTraits.flags & flag)) != 0u);
	}

	virtual bool collidesWith(vector2 point) { return _position == point; } // issa joke
	virtual bool collidesWith(const Collidable* collidable) { return this->collidesWith(collidable->getPosition()); }

	virtual ~Collidable(void) = default;
};

#endif
