#pragma once

#include "CollisionSystem.h"
#include "Types.h"

class Collidable;

namespace Kinematics2D
{
bool isSquareOnly(const Collidable* collidable);
bool hasPolygonSurface(const Collidable* collidable);
bool tryGetBounds(const Collidable* collidable, vector2& outMin, vector2& outMax);
bool tryGetActiveBounds(const Collidable* collidable, vector2& outMin, vector2& outMax);
bool sampleSupportY(const Collidable* collidable, float sampleX, float& outY);

bool hasWalkableSupportNearFoot(
	const Collidable* supportCollidable,
	float footMinX,
	float footMaxX,
	float footY,
	float& outSupportY,
	float footAboveTolerance,
	float footBelowTolerance,
	int minimumSamples);

bool isGroundContact(
	const CollisionContact& contact,
	float groundNormalThreshold,
	float footAboveTolerance,
	float footBelowTolerance);

bool isWallBlockingContact(
	const CollisionContact& contact,
	int horizontalIntent,
	float wallNormalThreshold);

bool shouldResolveAsOneWay(
	const Collidable* oneWayCollidable,
	const Collidable* dynamicCollidable,
	const vector2& currentPosition,
	const vector2& previousPosition,
	float oneWayEpsilon);

bool shouldPreferVerticalSeparation(
	const Collidable* dynamicCollidable,
	const Collidable* staticCollidable,
	float maxSquareStepHeight,
	float footAboveTolerance,
	float footBelowTolerance,
	int minimumSamples);
} // namespace Kinematics2D
