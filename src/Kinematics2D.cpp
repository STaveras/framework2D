#include "Kinematics2D.h"

#include "Collidable.h"
#include "CollidableGroup.h"
#include "Polygon.h"
#include "Square.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
constexpr float kAxisEpsilon = 0.0001f;
constexpr float kHorizontalSeparationEpsilon = 0.01f;

bool tryGetSquareBounds(const Collidable* collidable, vector2& outMin, vector2& outMax)
{
	if (!collidable || collidable->getType() != COL_OBJ_SQUARE) {
		return false;
	}

	const Square* square = (const Square*)collidable;
	outMin = square->getMin();
	outMax = square->getMax();
	return true;
}

bool tryGetPolygonBounds(const Collidable* collidable, vector2& outMin, vector2& outMax)
{
	if (!collidable || collidable->getType() != COL_OBJ_POLYGON) {
		return false;
	}

	const PolygonCollider* polygon = (const PolygonCollider*)collidable;
	if (!polygon || !polygon->isValid()) {
		return false;
	}

	outMin = polygon->getMin();
	outMax = polygon->getMax();
	return true;
}

bool hasPolygonSurfaceInternal(const Collidable* collidable)
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	if (collidable->getType() == COL_OBJ_POLYGON) {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		return polygon && polygon->isValid();
	}

	if (collidable->getType() != COL_OBJ_GROUP) {
		return false;
	}

	const CollidableGroup* group = (const CollidableGroup*)collidable;
	if (!group) {
		return false;
	}

	for (const Collidable* member : *group) {
		if (hasPolygonSurfaceInternal(member)) {
			return true;
		}
	}

	return false;
}

bool isSquareOnlyInternal(const Collidable* collidable)
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	switch (collidable->getType()) {
	case COL_OBJ_SQUARE:
		return true;
	case COL_OBJ_GROUP: {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group || group->empty()) {
			return false;
		}

		for (const Collidable* member : *group) {
			if (!isSquareOnlyInternal(member)) {
				return false;
			}
		}

		return true;
	}
	default:
		return false;
	}
}

} // namespace

namespace Kinematics2D
{

bool isSquareOnly(const Collidable* collidable)
{
	return isSquareOnlyInternal(collidable);
}

bool hasPolygonSurface(const Collidable* collidable)
{
	return hasPolygonSurfaceInternal(collidable);
}

bool tryGetBounds(const Collidable* collidable, vector2& outMin, vector2& outMax)
{
	if (!collidable) {
		return false;
	}

	if (tryGetSquareBounds(collidable, outMin, outMax) || tryGetPolygonBounds(collidable, outMin, outMax)) {
		return true;
	}

	if (collidable->getType() != COL_OBJ_GROUP) {
		return false;
	}

	const CollidableGroup* group = (const CollidableGroup*)collidable;
	if (!group || group->empty()) {
		return false;
	}

	bool hasAnyBounds = false;
	vector2 minBounds(0.0f, 0.0f);
	vector2 maxBounds(0.0f, 0.0f);
	for (const Collidable* member : *group) {
		vector2 memberMin(0.0f, 0.0f);
		vector2 memberMax(0.0f, 0.0f);
		if (!member || !tryGetBounds(member, memberMin, memberMax)) {
			continue;
		}

		if (!hasAnyBounds) {
			minBounds = memberMin;
			maxBounds = memberMax;
			hasAnyBounds = true;
			continue;
		}

		minBounds.x = std::min(minBounds.x, memberMin.x);
		minBounds.y = std::min(minBounds.y, memberMin.y);
		maxBounds.x = std::max(maxBounds.x, memberMax.x);
		maxBounds.y = std::max(maxBounds.y, memberMax.y);
	}

	if (!hasAnyBounds) {
		return false;
	}

	outMin = minBounds;
	outMax = maxBounds;
	return true;
}

bool tryGetActiveBounds(const Collidable* collidable, vector2& outMin, vector2& outMax)
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	return tryGetBounds(collidable, outMin, outMax);
}

bool sampleSupportY(const Collidable* collidable, float sampleX, float& outY)
{
	if (!collidable || !collidable->isActive()) {
		return false;
	}

	switch (collidable->getType()) {
	case COL_OBJ_SQUARE: {
		const Square* square = (const Square*)collidable;
		if (!square) {
			return false;
		}

		const vector2 min = square->getMin();
		const vector2 max = square->getMax();
		if (sampleX < min.x || sampleX > max.x) {
			return false;
		}

		outY = min.y;
		return true;
	}
	case COL_OBJ_POLYGON: {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (!polygon) {
			return false;
		}

		if (!polygon->isValid()) {
			float supportY = 0.0f;
			if (polygon->findTopSurfaceYAtX(sampleX, supportY)) {
				outY = supportY;
				return true;
			}
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
			if (!sampleSupportY(member, sampleX, memberY)) {
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

bool hasWalkableSupportNearFoot(
	const Collidable* supportCollidable,
	float footMinX,
	float footMaxX,
	float footY,
	float& outSupportY,
	float footAboveTolerance,
	float footBelowTolerance,
	int minimumSamples)
{
	if (!supportCollidable || !supportCollidable->isActive() || footMaxX <= footMinX) {
		return false;
	}

	if (!supportCollidable->hasSurfaceFlag(SurfaceFlags::Walkable)) {
		return false;
	}

	const float width = footMaxX - footMinX;
	const float sampleFractions[] = { 0.0f, 0.08f, 0.22f, 0.50f, 0.78f, 0.92f, 1.0f };
	const float sampleOffsets[] = { -1.5f, 0.0f, 1.5f };
	bool foundSupport = false;
	int supportSampleCount = 0;
	float bestDeltaMagnitude = std::numeric_limits<float>::max();
	float bestSupportY = 0.0f;

	for (float fraction : sampleFractions) {
		const float baseSampleX = footMinX + (width * fraction);
		for (float offset : sampleOffsets) {
			const float sampleX = std::clamp(baseSampleX + offset, footMinX, footMaxX);
			float supportY = 0.0f;
			if (!sampleSupportY(supportCollidable, sampleX, supportY)) {
				continue;
			}

			const float supportDelta = footY - supportY;
			if (supportDelta < -footAboveTolerance || supportDelta > footBelowTolerance) {
				continue;
			}

			++supportSampleCount;
			const float deltaMagnitude = std::fabs(supportDelta);
			if (!foundSupport || deltaMagnitude < bestDeltaMagnitude) {
				foundSupport = true;
				bestDeltaMagnitude = deltaMagnitude;
				bestSupportY = supportY;
			}
		}
	}

	if (!foundSupport || supportSampleCount < minimumSamples) {
		return false;
	}

	outSupportY = bestSupportY;
	return true;
}

bool isGroundContact(
	const CollisionContact& contact,
	float groundNormalThreshold,
	float footAboveTolerance,
	float footBelowTolerance)
{
	if (!contact.other || contact.phase == CollisionPhase::Exit) {
		return false;
	}

	const Collidable* supportCollidable = contact.otherCollidable;
	if (!supportCollidable || !supportCollidable->isActive() || !supportCollidable->hasSurfaceFlag(SurfaceFlags::Walkable)) {
		return false;
	}

	if (contact.normal.has_value() && contact.normal->y > groundNormalThreshold) {
		return true;
	}

	if (contact.separation.has_value() && contact.separation->y < -groundNormalThreshold) {
		return true;
	}

	const Collidable* selfCollidable = contact.selfCollidable;
	if (!selfCollidable || !selfCollidable->isActive()) {
		return false;
	}

	vector2 selfMin(0.0f, 0.0f);
	vector2 selfMax(0.0f, 0.0f);
	if (!tryGetBounds(selfCollidable, selfMin, selfMax)) {
		return false;
	}

	const float bodyBottom = selfMax.y;
	const float bodyWidth = selfMax.x - selfMin.x;
	if (bodyWidth <= 0.0f) {
		return false;
	}

	const float sampleFractions[] = { 0.10f, 0.25f, 0.50f, 0.75f, 0.90f };
	const float sampleOffsets[] = { -5.0f, -3.0f, -1.0f, 0.0f, 1.0f, 3.0f, 5.0f };
	for (float fraction : sampleFractions) {
		for (float offset : sampleOffsets) {
			const float sampleX = selfMin.x + (bodyWidth * fraction) + offset;
			float supportY = 0.0f;
			if (!sampleSupportY(supportCollidable, sampleX, supportY)) {
				continue;
			}

			const float supportDelta = bodyBottom - supportY;
			if (supportDelta >= -footAboveTolerance && supportDelta <= footBelowTolerance) {
				return true;
			}
		}
	}

	return false;
}

bool isWallBlockingContact(
	const CollisionContact& contact,
	int horizontalIntent,
	float wallNormalThreshold)
{
	if (horizontalIntent == 0 || !contact.normal.has_value() || !contact.otherCollidable) {
		return false;
	}

	if (contact.otherCollidable->hasSurfaceFlag(SurfaceFlags::Walkable)) {
		return false;
	}

	const vector2 normal = contact.normal.value();
	const float absNormalX = std::fabs(normal.x);
	const float absNormalY = std::fabs(normal.y);
	if (absNormalX <= wallNormalThreshold || absNormalX <= absNormalY) {
		return false;
	}

	const bool pushingIntoWall =
		(horizontalIntent > 0 && normal.x > 0.0f) ||
		(horizontalIntent < 0 && normal.x < 0.0f);
	if (!pushingIntoWall) {
		return false;
	}

	if (contact.separation.has_value()) {
		const vector2 separation = contact.separation.value();
		if (std::fabs(separation.x) <= (std::fabs(separation.y) + kHorizontalSeparationEpsilon)) {
			return false;
		}
	}

	return true;
}

bool shouldResolveAsOneWay(
	const Collidable* oneWayCollidable,
	const Collidable* dynamicCollidable,
	const vector2& currentPosition,
	const vector2& previousPosition,
	float oneWayEpsilon)
{
	if (!oneWayCollidable || !dynamicCollidable) {
		return true;
	}

	if (!oneWayCollidable->hasSurfaceFlag(SurfaceFlags::OneWay)) {
		return true;
	}

	vector2 dynamicMin(0.0f, 0.0f);
	vector2 dynamicMax(0.0f, 0.0f);
	if (!tryGetBounds(dynamicCollidable, dynamicMin, dynamicMax)) {
		return true;
	}

	const float sampleX = dynamicMin.x + ((dynamicMax.x - dynamicMin.x) * 0.5f);
	float supportY = 0.0f;
	if (!sampleSupportY(oneWayCollidable, sampleX, supportY)) {
		return true;
	}

	const float deltaY = currentPosition.y - previousPosition.y;
	if (deltaY < -kAxisEpsilon) {
		return false;
	}

	const float currentBottom = dynamicMax.y;
	const float previousBottom = currentBottom - deltaY;

	const bool crossedPlatformTop =
		(previousBottom <= (supportY + oneWayEpsilon)) &&
		(currentBottom >= (supportY - oneWayEpsilon));
	const bool restingOnTop =
		(std::fabs(currentBottom - supportY) <= (oneWayEpsilon * 2.0f)) &&
		(previousBottom <= (supportY + oneWayEpsilon * 2.0f));

	return crossedPlatformTop || restingOnTop;
}

bool shouldPreferVerticalSeparation(
	const Collidable* dynamicCollidable,
	const Collidable* staticCollidable,
	float maxSquareStepHeight,
	float footAboveTolerance,
	float footBelowTolerance,
	int minimumSamples)
{
	if (!dynamicCollidable || !staticCollidable || !staticCollidable->hasSurfaceFlag(SurfaceFlags::Walkable)) {
		return false;
	}

	vector2 dynamicMin(0.0f, 0.0f);
	vector2 dynamicMax(0.0f, 0.0f);
	if (!tryGetBounds(dynamicCollidable, dynamicMin, dynamicMax)) {
		return false;
	}

	float supportY = 0.0f;
	if (!hasWalkableSupportNearFoot(
			staticCollidable,
			dynamicMin.x,
			dynamicMax.x,
			dynamicMax.y,
			supportY,
			footAboveTolerance,
			footBelowTolerance,
			minimumSamples)) {
		return false;
	}

	const float supportDelta = dynamicMax.y - supportY;
	if (hasPolygonSurfaceInternal(staticCollidable)) {
		return true;
	}

	if (!staticCollidable->hasSurfaceFlag(SurfaceFlags::StepCandidate)) {
		return false;
	}

	return supportDelta >= 0.0f && supportDelta <= maxSquareStepHeight;
}

} // namespace Kinematics2D
