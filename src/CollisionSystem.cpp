#include "CollisionSystem.h"

#include "Collidable.h"
#include "CollidableGroup.h"
#include "Debug.h"
#include "GameObject.h"
#include "Polygon.h"
#include "Kinematics2D.h"
#include "Renderable.h"
#include "Square.h"
#include "Tile.h"
#include "TileSet.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
constexpr float kAxisEpsilon = 0.0001f;
constexpr float kOneWayEpsilon = 0.5f;
constexpr float kSeparationSlop = 0.001f;
constexpr float kTouchContactEpsilon = 0.01f;
constexpr float kCcdStepPixels = 2.5f;
constexpr int kCcdMaxSubsteps = 16;
constexpr int kResolveIterations = 8;
constexpr float kSupportProbeFootAboveTolerance = 2.0f;
constexpr float kSupportProbeFootBelowTolerance = 20.0f;
constexpr int kMinimumSupportSamplesForWalkable = 1;
constexpr float kMaxSquareStepUpForVerticalSeparation = 8.0f;

int phasePriority(CollisionPhase phase)
{
	switch (phase) {
	case CollisionPhase::Enter:
		return 3;
	case CollisionPhase::Stay:
		return 2;
	case CollisionPhase::Exit:
		return 1;
	default:
		return 0;
	}
}

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
	// Allow both convex and non-convex (but valid) polygons - we can still sample their surfaces
	if (!polygon || !polygon->isValid()) {
		return false;
	}

	outMin = polygon->getMin();
	outMax = polygon->getMax();
	return true;
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

bool tryComputeTouchingNormal(const Collidable* firstCollidable, const Collidable* secondCollidable, vector2& outNormal)
{
	vector2 firstMin(0.0f, 0.0f);
	vector2 firstMax(0.0f, 0.0f);
	vector2 secondMin(0.0f, 0.0f);
	vector2 secondMax(0.0f, 0.0f);
	if (!tryGetBounds(firstCollidable, firstMin, firstMax) || !tryGetBounds(secondCollidable, secondMin, secondMax)) {
		return false;
	}

	const float overlapX = std::min(firstMax.x, secondMax.x) - std::max(firstMin.x, secondMin.x);
	const float overlapY = std::min(firstMax.y, secondMax.y) - std::max(firstMin.y, secondMin.y);
	if (overlapX >= 0.0f && overlapY >= 0.0f) {
		return false;
	}

	if (overlapX < -kTouchContactEpsilon || overlapY < -kTouchContactEpsilon) {
		return false;
	}

	const vector2 firstCenter = firstMin + ((firstMax - firstMin) * 0.5f);
	const vector2 secondCenter = secondMin + ((secondMax - secondMin) * 0.5f);
	if (overlapX >= 0.0f && overlapY < 0.0f) {
		outNormal = (secondCenter.y >= firstCenter.y) ? vector2(0.0f, 1.0f) : vector2(0.0f, -1.0f);
		return true;
	}
	if (overlapY >= 0.0f && overlapX < 0.0f) {
		outNormal = (secondCenter.x >= firstCenter.x) ? vector2(1.0f, 0.0f) : vector2(-1.0f, 0.0f);
		return true;
	}

	return false;
}

void collectPolygonLoops(const Collidable* collidable, std::vector<std::vector<vector2>>& outLoops)
{
	if (!collidable) {
		return;
	}

	if (collidable->getType() == COL_OBJ_POLYGON) {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (polygon && polygon->isValid()) {
			const std::vector<vector2>& vertices = polygon->getWorldVertices();
			if (vertices.size() >= 3) {
				outLoops.push_back(vertices);
			}
		}
		return;
	}

	if (collidable->getType() == COL_OBJ_GROUP) {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group) {
			return;
		}
		for (const Collidable* member : *group) {
			collectPolygonLoops(member, outLoops);
		}
	}
}

void collectPrimitiveCollidables(const Collidable* collidable, std::vector<const Collidable*>& outPrimitives)
{
	if (!collidable) {
		return;
	}

	if (collidable->getType() == COL_OBJ_GROUP) {
		const CollidableGroup* group = (const CollidableGroup*)collidable;
		if (!group) {
			return;
		}
		for (const Collidable* member : *group) {
			collectPrimitiveCollidables(member, outPrimitives);
		}
		return;
	}

	outPrimitives.push_back(collidable);
}

void buildSquareVertices(const Square* square, std::vector<vector2>& outVertices)
{
	outVertices.clear();
	if (!square) {
		return;
	}

	const vector2 min = square->getMin();
	const vector2 max = square->getMax();
	outVertices.push_back(vector2(min.x, min.y));
	outVertices.push_back(vector2(max.x, min.y));
	outVertices.push_back(vector2(max.x, max.y));
	outVertices.push_back(vector2(min.x, max.y));
}

vector2 computeCentroid(const std::vector<vector2>& vertices)
{
	if (vertices.empty()) {
		return vector2(0.0f, 0.0f);
	}

	vector2 centroid(0.0f, 0.0f);
	for (const vector2& vertex : vertices) {
		centroid.x += vertex.x;
		centroid.y += vertex.y;
	}
	return vector2(
		centroid.x / (float)vertices.size(),
		centroid.y / (float)vertices.size());
}

bool getVertices(const Collidable* collidable, std::vector<vector2>& outVertices)
{
	outVertices.clear();
	if (!collidable) {
		return false;
	}

	if (collidable->getType() == COL_OBJ_SQUARE) {
		buildSquareVertices((const Square*)collidable, outVertices);
		return outVertices.size() == 4;
	}
	if (collidable->getType() == COL_OBJ_POLYGON) {
		const PolygonCollider* polygon = (const PolygonCollider*)collidable;
		if (!polygon || !polygon->isValid()) {
			return false;
		}
		outVertices = polygon->getWorldVertices();
		return outVertices.size() >= 3;
	}

	return false;
}

bool projectOnAxis(const std::vector<vector2>& vertices, const vector2& axis, float& outMin, float& outMax)
{
	if (vertices.empty()) {
		return false;
	}

	outMin = dot(vertices[0], axis);
	outMax = outMin;
	for (size_t i = 1; i < vertices.size(); ++i) {
		const float value = dot(vertices[i], axis);
		outMin = std::min(outMin, value);
		outMax = std::max(outMax, value);
	}
	return true;
}

bool computeSATHints(
	const std::vector<vector2>& aVertices,
	const std::vector<vector2>& bVertices,
	std::optional<vector2>& outNormal,
	std::optional<float>& outPenetrationDepth)
{
	if (aVertices.size() < 3 || bVertices.size() < 3) {
		return false;
	}

	float minOverlap = std::numeric_limits<float>::max();
	vector2 bestAxis(0.0f, 0.0f);
	bool hasAxis = false;

	auto testAxes = [&](const std::vector<vector2>& vertices) -> bool {
		for (size_t i = 0; i < vertices.size(); ++i) {
			const vector2& p0 = vertices[i];
			const vector2& p1 = vertices[(i + 1) % vertices.size()];
			vector2 edge = p1 - p0;
			if (edge.length() <= kAxisEpsilon) {
				continue;
			}

			vector2 axis(-edge.y, edge.x);
			axis.normalize();

			float aMin = 0.0f;
			float aMax = 0.0f;
			float bMin = 0.0f;
			float bMax = 0.0f;
			if (!projectOnAxis(aVertices, axis, aMin, aMax) || !projectOnAxis(bVertices, axis, bMin, bMax)) {
				return false;
			}

			const float overlap = std::min(aMax, bMax) - std::max(aMin, bMin);
			if (overlap <= 0.0f) {
				return false;
			}

			if (overlap < minOverlap) {
				minOverlap = overlap;
				bestAxis = axis;
				hasAxis = true;
			}
		}
		return true;
	};

	if (!testAxes(aVertices) || !testAxes(bVertices) || !hasAxis) {
		return false;
	}

	vector2 centerA = computeCentroid(aVertices);
	vector2 centerB = computeCentroid(bVertices);
	vector2 toB = centerB - centerA;
	if (dot(toB, bestAxis) < 0.0f) {
		bestAxis = vector2(-bestAxis.x, -bestAxis.y);
	}

	outNormal = bestAxis;
	outPenetrationDepth = minOverlap;
	return true;
}

std::optional<vector2> computeContactMidpoint(const Collidable* a, const Collidable* b, bool overlapping)
{
	if (!a || !b) {
		return std::nullopt;
	}

	if (overlapping) {
		vector2 aMin, aMax, bMin, bMax;
		if (tryGetBounds(a, aMin, aMax) && tryGetBounds(b, bMin, bMax)) {
			vector2 overlapMin(
				std::max(aMin.x, bMin.x),
				std::max(aMin.y, bMin.y));
			vector2 overlapMax(
				std::min(aMax.x, bMax.x),
				std::min(aMax.y, bMax.y));

			if (overlapMax.x > overlapMin.x && overlapMax.y > overlapMin.y) {
				return overlapMin + ((overlapMax - overlapMin) * 0.5f);
			}
		}
	}

	return a->getPosition() + ((b->getPosition() - a->getPosition()) * 0.5f);
}

void updateContactPhase(std::map<GameObject*, CollisionPhase>& phaseMap, GameObject* object, CollisionPhase phase)
{
	if (!object) {
		return;
	}

	auto itr = phaseMap.find(object);
	if (itr == phaseMap.end() || phasePriority(phase) > phasePriority(itr->second)) {
		phaseMap[object] = phase;
	}
}

bool shouldForceVerticalSeparationForWalkablePolygon(
	GameObject* dynamicObject,
	GameObject* staticObject,
	const Collidable* dynamicCollidable,
	const Collidable* staticCollidable,
	const vector2& axis)
{
	if (!dynamicObject || !staticObject || !dynamicCollidable || !staticCollidable) {
		return false;
	}

	if (!staticObject->isStatic()) {
		return false;
	}

	(void)axis;
	return Kinematics2D::shouldPreferVerticalSeparation(
		dynamicCollidable,
		staticCollidable,
		kMaxSquareStepUpForVerticalSeparation,
		kSupportProbeFootAboveTolerance,
		kSupportProbeFootBelowTolerance,
		kMinimumSupportSamplesForWalkable);
}

bool shouldResolveAsOneWay(
	GameObject* first,
	GameObject* second,
	const Collidable* firstCollidable,
	const Collidable* secondCollidable,
	const std::map<GameObject*, vector2>& previousStepPositions)
{
	if (!first || !second || !firstCollidable || !secondCollidable) {
		return false;
	}

	const Collidable* oneWayCollidable = nullptr;
	GameObject* dynamicObject = nullptr;
	const Collidable* dynamicCollidable = nullptr;

	if (first->isStatic() && firstCollidable->hasSurfaceFlag(SurfaceFlags::OneWay) && !second->isStatic()) {
		oneWayCollidable = firstCollidable;
		dynamicObject = second;
		dynamicCollidable = secondCollidable;
	}
	else if (second->isStatic() && secondCollidable->hasSurfaceFlag(SurfaceFlags::OneWay) && !first->isStatic()) {
		oneWayCollidable = secondCollidable;
		dynamicObject = first;
		dynamicCollidable = firstCollidable;
	}

	if (!oneWayCollidable || !dynamicObject || !dynamicCollidable) {
		return true;
	}

	const vector2 currentPosition = dynamicObject->getPosition();
	vector2 previousPosition = currentPosition;
	auto previousItr = previousStepPositions.find(dynamicObject);
	if (previousItr != previousStepPositions.end()) {
		previousPosition = previousItr->second;
	}

	return Kinematics2D::shouldResolveAsOneWay(
		oneWayCollidable,
		dynamicCollidable,
		currentPosition,
		previousPosition,
		kOneWayEpsilon);
}

void clipVelocityAlongAxis(GameObject* object, const vector2& intoNormal)
{
	if (!object) {
		return;
	}

	vector2 normal = intoNormal;
	if (normal.length() <= kAxisEpsilon) {
		return;
	}
	normal.normalize();

	vector2 velocity = object->getVelocity();
	const float intoSurface = dot(velocity, normal);
	if (intoSurface > 0.0f) {
		vector2 clippedVelocity = velocity - (normal * intoSurface);

		// Gameplay controller expectation: ground response should not inject
		// upward lift from purely horizontal motion on ramps.
		if (normal.y > 0.2f && velocity.y >= -kAxisEpsilon && clippedVelocity.y < 0.0f) {
			clippedVelocity.y = 0.0f;
		}

		object->setVelocity(clippedVelocity);
	}
}

} // namespace

bool CollisionSystem::CollisionPairKey::operator<(const CollisionPairKey& rhs) const
{
	if (first != rhs.first) {
		return std::less<GameObject*>()(first, rhs.first);
	}
	return std::less<GameObject*>()(second, rhs.second);
}

CollisionSystem::CollisionPairKey CollisionSystem::makePairKey(GameObject* a, GameObject* b)
{
	if (std::less<GameObject*>()(b, a)) {
		std::swap(a, b);
	}

	CollisionPairKey key;
	key.first = a;
	key.second = b;
	return key;
}

void CollisionSystem::computeGeometryHints(const Collidable* a, const Collidable* b, std::optional<vector2>& normal, std::optional<float>& penetrationDepth)
{
	if (!a || !b) {
		return;
	}

	std::vector<const Collidable*> aPrimitives;
	std::vector<const Collidable*> bPrimitives;
	collectPrimitiveCollidables(a, aPrimitives);
	collectPrimitiveCollidables(b, bPrimitives);

	if (aPrimitives.empty() || bPrimitives.empty()) {
		return;
	}

	float bestDepth = std::numeric_limits<float>::max();
	std::optional<vector2> bestNormal;
	std::optional<float> bestPenetration;
	std::vector<vector2> aVertices;
	std::vector<vector2> bVertices;
	for (const Collidable* aPrimitive : aPrimitives) {
		if (!aPrimitive) {
			continue;
		}

		for (const Collidable* bPrimitive : bPrimitives) {
			if (!bPrimitive) {
				continue;
			}

			if (!getVertices(aPrimitive, aVertices) || !getVertices(bPrimitive, bVertices)) {
				continue;
			}

			std::optional<vector2> candidateNormal;
			std::optional<float> candidateDepth;
			if (!computeSATHints(aVertices, bVertices, candidateNormal, candidateDepth)) {
				continue;
			}

			const float depthValue = candidateDepth.value_or(std::numeric_limits<float>::max());
			if (depthValue < bestDepth) {
				bestDepth = depthValue;
				bestNormal = candidateNormal;
				bestPenetration = candidateDepth;
			}
		}
	}

	if (bestNormal.has_value() && bestPenetration.has_value()) {
		normal = bestNormal;
		penetrationDepth = bestPenetration;
	}
}

void CollisionSystem::dispatchPair(
	GameObject* a,
	GameObject* b,
	Collidable* aCollidable,
	Collidable* bCollidable,
	CollisionPhase phase,
	bool overlapping,
	const std::optional<vector2>& normalHint,
	const std::optional<float>& penetrationHint,
	const std::optional<float>& timeOfImpact,
	const std::optional<vector2>& separation) const
{
	if (!a || !b) {
		return;
	}

	std::optional<vector2> normal = normalHint;
	std::optional<float> penetrationDepth = penetrationHint;
	if (overlapping && !normal) {
		computeGeometryHints(aCollidable, bCollidable, normal, penetrationDepth);
	}

	CollisionContact contactA;
	contactA.self = a;
	contactA.other = b;
	contactA.selfCollidable = aCollidable;
	contactA.otherCollidable = bCollidable;
	contactA.phase = phase;
	contactA.overlapping = overlapping;
	contactA.normal = normal;
	contactA.penetrationDepth = penetrationDepth;
	contactA.timeOfImpact = timeOfImpact;
	if (separation.has_value()) {
		contactA.separation = separation;
	}
	a->onCollisionContact(contactA);

	CollisionContact contactB;
	contactB.self = b;
	contactB.other = a;
	contactB.selfCollidable = bCollidable;
	contactB.otherCollidable = aCollidable;
	contactB.phase = phase;
	contactB.overlapping = overlapping;
	if (normal.has_value()) {
		contactB.normal = vector2(-normal->x, -normal->y);
	}
	contactB.penetrationDepth = penetrationDepth;
	contactB.timeOfImpact = timeOfImpact;
	if (separation.has_value()) {
		contactB.separation = vector2(-separation->x, -separation->y);
	}
	b->onCollisionContact(contactB);
}

void CollisionSystem::reset(void)
{
	_activePairs.clear();
	_debugShapes.clear();
	_debugContacts.clear();
	_previousPositions.clear();
}

void CollisionSystem::update(const std::map<std::string, GameObject*>& objects, float dt)
{
	struct PairRuntimeInfo
	{
		std::optional<float> timeOfImpact;
		std::optional<vector2> normal;
		std::optional<float> penetrationDepth;
		std::optional<vector2> separation;
	};

	struct PendingDispatch
	{
		GameObject* first = nullptr;
		GameObject* second = nullptr;
		CollisionPhase phase = CollisionPhase::Stay;
		bool overlapping = false;
		std::optional<vector2> normal;
		std::optional<float> penetrationDepth;
		std::optional<float> timeOfImpact;
		std::optional<vector2> separation;
	};

	const bool collectDebugData = DEBUGGING && Debug::dbgCollision;

	std::vector<GameObject*> activeObjects;
	activeObjects.reserve(objects.size());

	std::unordered_set<GameObject*> activeObjectSet;
	activeObjectSet.reserve(objects.size());

	for (const auto& entry : objects) {
		GameObject* object = entry.second;
		if (!object) {
			continue;
		}
		activeObjects.push_back(object);
		activeObjectSet.insert(object);
	}

	for (auto itr = _previousPositions.begin(); itr != _previousPositions.end();) {
		if (activeObjectSet.find(itr->first) == activeObjectSet.end()) {
			itr = _previousPositions.erase(itr);
		}
		else {
			++itr;
		}
	}

	std::map<GameObject*, vector2> frameStartPositions;
	std::map<GameObject*, vector2> frameTargetPositions;
	std::vector<GameObject*> dynamicObjects;
	std::vector<GameObject*> staticObjects;
	dynamicObjects.reserve(activeObjects.size());
	staticObjects.reserve(activeObjects.size());

	float maxDisplacement = 0.0f;
	int inferredStartCount = 0;
	for (GameObject* object : activeObjects) {
		if (!object) {
			continue;
		}

		Collidable* collidable = object->getCollidable();
		if (!collidable || !collidable->isActive()) {
			continue;
		}

		if (object->getType() == GameObject::GAME_OBJ_TILE) {
			const Tile* tile = (const Tile*)object;
			if (tile && tile->isNonCollidingLayer()) {
				continue;
			}
		}

		const vector2 currentPosition = object->getPosition();
		auto previousItr = _previousPositions.find(object);
		if (previousItr == _previousPositions.end()) {
			vector2 inferredStart = currentPosition;
			// First frame for an object: approximate pre-integration position to avoid
			// dropping the initial sweep and tunneling on startup spikes.
			if (!object->isStatic() && dt > 0.0f) {
				inferredStart = currentPosition - (object->getVelocity() * dt);
				++inferredStartCount;
			}
			_previousPositions[object] = inferredStart;
			previousItr = _previousPositions.find(object);
		}

		frameStartPositions[object] = previousItr->second;
		frameTargetPositions[object] = currentPosition;

		if (object->isStatic()) {
			staticObjects.push_back(object);
		}
		else {
			dynamicObjects.push_back(object);
			const float displacement = (currentPosition - previousItr->second).length();
			maxDisplacement = std::max(maxDisplacement, displacement);
		}
	}

	const int substeps = std::max(
		1,
		std::min(
			kCcdMaxSubsteps,
			(int)std::ceil(maxDisplacement / std::max(0.01f, kCcdStepPixels))));

	for (GameObject* object : dynamicObjects) {
		auto startItr = frameStartPositions.find(object);
		if (startItr != frameStartPositions.end()) {
			object->setPosition(startItr->second);
		}
	}

	std::map<CollisionPairKey, PairRuntimeInfo> runtimePairInfo;
	float maxCorrectionMagnitude = 0.0f;

	for (int substep = 1; substep <= substeps; ++substep) {
		const float alpha = (float)substep / (float)substeps;
		std::map<GameObject*, vector2> previousStepPositions;

		for (GameObject* object : dynamicObjects) {
			if (!object) {
				continue;
			}
			previousStepPositions[object] = object->getPosition();

			auto startItr = frameStartPositions.find(object);
			auto targetItr = frameTargetPositions.find(object);
			if (startItr == frameStartPositions.end() || targetItr == frameTargetPositions.end()) {
				continue;
			}

			const vector2 interpolated = startItr->second + ((targetItr->second - startItr->second) * alpha);
			object->setPosition(interpolated);
		}

		for (int iteration = 0; iteration < kResolveIterations; ++iteration) {
			bool resolvedAnyPair = false;

			auto resolvePair = [&](GameObject* first, GameObject* second) {
				if (!first || !second || first == second) {
					return;
				}

				Collidable* firstCollidable = first->getCollidable();
				Collidable* secondCollidable = second->getCollidable();
				if (!firstCollidable || !secondCollidable || !firstCollidable->isActive() || !secondCollidable->isActive()) {
					return;
				}

				if (!first->shouldCollideWith(*second) || !second->shouldCollideWith(*first)) {
					return;
				}

				if (!shouldResolveAsOneWay(first, second, firstCollidable, secondCollidable, previousStepPositions)) {
					return;
				}

				const bool overlapping =
					firstCollidable->collidesWith(secondCollidable) ||
					secondCollidable->collidesWith(firstCollidable);
				if (!overlapping) {
					return;
				}

				CollisionPairKey key = makePairKey(first, second);
				PairRuntimeInfo& pairInfo = runtimePairInfo[key];
				if (!pairInfo.timeOfImpact.has_value()) {
					pairInfo.timeOfImpact = (float)substep / (float)substeps;
				}

				std::optional<vector2> normal;
				std::optional<float> penetrationDepth;
				computeGeometryHints(firstCollidable, secondCollidable, normal, penetrationDepth);
				if (!normal.has_value() || !penetrationDepth.has_value() || penetrationDepth.value() <= 0.0f) {
					return;
				}

				vector2 axis = normal.value();
				if (axis.length() <= kAxisEpsilon) {
					return;
				}
				axis.normalize();

				const float depth = penetrationDepth.value() + kSeparationSlop;
				if (depth <= 0.0f) {
					return;
				}

					const bool firstStatic = first->isStatic();
					const bool secondStatic = second->isStatic();
					if (firstStatic && secondStatic) {
						return;
					}

					vector2 resolveAxis = axis;
					if (secondStatic && !firstStatic &&
						shouldForceVerticalSeparationForWalkablePolygon(
							first,
							second,
							firstCollidable,
							secondCollidable,
							axis)) {
						resolveAxis = vector2(0.0f, 1.0f);
					}
					else if (firstStatic && !secondStatic &&
							 shouldForceVerticalSeparationForWalkablePolygon(
								 second,
								 first,
								 secondCollidable,
								 firstCollidable,
								 vector2(-axis.x, -axis.y))) {
						resolveAxis = vector2(0.0f, -1.0f);
					}

					vector2 moveFirst(0.0f, 0.0f);
					vector2 moveSecond(0.0f, 0.0f);
					if (firstStatic) {
						moveSecond = resolveAxis * depth;
					}
					else if (secondStatic) {
						moveFirst = vector2(-resolveAxis.x, -resolveAxis.y) * depth;
					}
					else {
						const float firstMass = std::max(0.0001f, first->getMass());
						const float secondMass = std::max(0.0001f, second->getMass());
						const float firstInvMass = 1.0f / firstMass;
						const float secondInvMass = 1.0f / secondMass;
						const float totalInvMass = std::max(0.0001f, firstInvMass + secondInvMass);

						const float firstShare = firstInvMass / totalInvMass;
						const float secondShare = secondInvMass / totalInvMass;
						moveFirst = vector2(-resolveAxis.x, -resolveAxis.y) * (depth * firstShare);
						moveSecond = resolveAxis * (depth * secondShare);
					}

					if (!firstStatic && moveFirst.length() > 0.0f) {
						first->setPosition(first->getPosition() + moveFirst);
						maxCorrectionMagnitude = std::max(maxCorrectionMagnitude, moveFirst.length());
						if (!(secondStatic && second->getType() == GameObject::GAME_OBJ_TILE)) {
							clipVelocityAlongAxis(first, resolveAxis);
						}
					}

					if (!secondStatic && moveSecond.length() > 0.0f) {
						second->setPosition(second->getPosition() + moveSecond);
						maxCorrectionMagnitude = std::max(maxCorrectionMagnitude, moveSecond.length());
						if (!(firstStatic && first->getType() == GameObject::GAME_OBJ_TILE)) {
							clipVelocityAlongAxis(second, vector2(-resolveAxis.x, -resolveAxis.y));
						}
					}

					vector2 axisForKeyOrder = resolveAxis;
					if (key.first != first) {
						axisForKeyOrder = vector2(-resolveAxis.x, -resolveAxis.y);
					}

				pairInfo.normal = axisForKeyOrder;
				pairInfo.penetrationDepth = penetrationDepth;
				pairInfo.separation = vector2(-axisForKeyOrder.x, -axisForKeyOrder.y) * penetrationDepth.value();
				resolvedAnyPair = true;
			};

			for (size_t i = 0; i < dynamicObjects.size(); ++i) {
				GameObject* first = dynamicObjects[i];
				for (size_t j = i + 1; j < dynamicObjects.size(); ++j) {
					resolvePair(first, dynamicObjects[j]);
				}
				for (GameObject* staticObject : staticObjects) {
					resolvePair(first, staticObject);
				}
			}

			if (!resolvedAnyPair) {
				break;
			}
		}
	}

	std::set<CollisionPairKey> currentPairs;
	std::vector<PendingDispatch> pendingDispatches;
	std::set<CollisionPairKey> enqueuedPairs;

	auto enqueuePair = [&](GameObject* first,
						   GameObject* second,
						   bool overlapping,
						   const PairRuntimeInfo* runtimeInfo,
						   const std::optional<vector2>& normalOverride = std::nullopt,
						   const std::optional<float>& penetrationOverride = std::nullopt,
						   const std::optional<vector2>& separationOverride = std::nullopt) {
		if (!first || !second || first == second) {
			return;
		}

		if (activeObjectSet.find(first) == activeObjectSet.end() ||
			activeObjectSet.find(second) == activeObjectSet.end()) {
			return;
		}

		Collidable* firstCollidable = first->getCollidable();
		Collidable* secondCollidable = second->getCollidable();
		if (!firstCollidable || !secondCollidable || !firstCollidable->isActive() || !secondCollidable->isActive()) {
			return;
		}

		if (!first->shouldCollideWith(*second) || !second->shouldCollideWith(*first)) {
			return;
		}

		if (!shouldResolveAsOneWay(first, second, firstCollidable, secondCollidable, frameStartPositions)) {
			return;
		}

		const CollisionPairKey key = makePairKey(first, second);
		if (enqueuedPairs.find(key) != enqueuedPairs.end()) {
			return;
		}

		enqueuedPairs.insert(key);
		currentPairs.insert(key);

		PendingDispatch pending;
		pending.first = first;
		pending.second = second;
		pending.phase = (_activePairs.find(key) == _activePairs.end()) ? CollisionPhase::Enter : CollisionPhase::Stay;
		pending.overlapping = overlapping;

		if (runtimeInfo) {
			pending.timeOfImpact = runtimeInfo->timeOfImpact;
			pending.normal = runtimeInfo->normal;
			pending.penetrationDepth = runtimeInfo->penetrationDepth;
			pending.separation = runtimeInfo->separation;
		}

		if ((!pending.normal.has_value() || !pending.penetrationDepth.has_value()) && overlapping) {
			computeGeometryHints(firstCollidable, secondCollidable, pending.normal, pending.penetrationDepth);
			if (pending.normal.has_value() && pending.penetrationDepth.has_value() && !pending.separation.has_value()) {
				pending.separation = vector2(-pending.normal->x, -pending.normal->y) * pending.penetrationDepth.value();
			}
		}

		if (normalOverride.has_value() && !pending.normal.has_value()) {
			pending.normal = normalOverride;
		}
		if (penetrationOverride.has_value() && !pending.penetrationDepth.has_value()) {
			pending.penetrationDepth = penetrationOverride;
		}
		if (separationOverride.has_value() && !pending.separation.has_value()) {
			pending.separation = separationOverride;
		}

		pendingDispatches.push_back(pending);
	};

	for (const auto& runtimePair : runtimePairInfo) {
		const CollisionPairKey& key = runtimePair.first;
		GameObject* first = key.first;
		GameObject* second = key.second;
		if (!first || !second) {
			continue;
		}

		Collidable* firstCollidable = first->getCollidable();
		Collidable* secondCollidable = second->getCollidable();
		const bool overlapping =
			firstCollidable &&
			secondCollidable &&
			firstCollidable->isActive() &&
			secondCollidable->isActive() &&
			(firstCollidable->collidesWith(secondCollidable) || secondCollidable->collidesWith(firstCollidable));

		enqueuePair(first, second, overlapping, &runtimePair.second);
	}

	auto enqueuePairIfOverlapping = [&](GameObject* first, GameObject* second) {
		if (!first || !second || first == second) {
			return;
		}

		Collidable* firstCollidable = first->getCollidable();
		Collidable* secondCollidable = second->getCollidable();
		if (!firstCollidable || !secondCollidable || !firstCollidable->isActive() || !secondCollidable->isActive()) {
			return;
		}

		if (!first->shouldCollideWith(*second) || !second->shouldCollideWith(*first)) {
			return;
		}

		if (!shouldResolveAsOneWay(first, second, firstCollidable, secondCollidable, frameStartPositions)) {
			return;
		}

		const bool overlapping = firstCollidable->collidesWith(secondCollidable) || secondCollidable->collidesWith(firstCollidable);
		std::optional<vector2> touchingNormal;
		if (!overlapping) {
			vector2 normal(0.0f, 0.0f);
			if (!tryComputeTouchingNormal(firstCollidable, secondCollidable, normal)) {
				return;
			}
			touchingNormal = normal;
		}

		CollisionPairKey key = makePairKey(first, second);
		auto runtimeInfoItr = runtimePairInfo.find(key);
		const PairRuntimeInfo* runtimeInfo = (runtimeInfoItr != runtimePairInfo.end()) ? &runtimeInfoItr->second : nullptr;
		enqueuePair(
			first,
			second,
			overlapping,
			runtimeInfo,
			touchingNormal,
			touchingNormal.has_value() ? std::optional<float>(0.0f) : std::nullopt,
			std::nullopt);
	};

	for (size_t i = 0; i < dynamicObjects.size(); ++i) {
		GameObject* first = dynamicObjects[i];
		for (size_t j = i + 1; j < dynamicObjects.size(); ++j) {
			enqueuePairIfOverlapping(first, dynamicObjects[j]);
		}
		for (GameObject* staticObject : staticObjects) {
			enqueuePairIfOverlapping(first, staticObject);
		}
	}

	for (const CollisionPairKey& previousPair : _activePairs) {
		if (currentPairs.find(previousPair) != currentPairs.end()) {
			continue;
		}

		if (activeObjectSet.find(previousPair.first) == activeObjectSet.end() ||
			activeObjectSet.find(previousPair.second) == activeObjectSet.end()) {
			continue;
		}

		PendingDispatch pending;
		pending.first = previousPair.first;
		pending.second = previousPair.second;
		pending.phase = CollisionPhase::Exit;
		pending.overlapping = false;
		pendingDispatches.push_back(pending);
	}

	_debugShapes.clear();
	_debugContacts.clear();
	std::map<GameObject*, size_t> shapeIndexByObject;
	if (collectDebugData) {
		_debugShapes.reserve(activeObjects.size());
		for (GameObject* object : activeObjects) {
			if (!object) {
				continue;
			}

			CollisionDebugShape shape;
			shape.object = object;
			shape.objectPosition = object->getPosition();
			shape.collisionAnchor = object->getCollisionAnchor();

			auto sweepStartItr = frameStartPositions.find(object);
			if (sweepStartItr != frameStartPositions.end()) {
				shape.hasSweep = true;
				shape.sweepStart = sweepStartItr->second;
				shape.sweepEnd = object->getPosition();
			}

			if (GameObject::GameObjectState* state = object->getState()) {
				if (Renderable* renderable = state->getRenderable()) {
					shape.renderableOffset = renderable->getOffset();
				}
			}

			shape.anchorWithRenderableOffset = shape.objectPosition + shape.renderableOffset;
			shape.collidable = object->getCollidable();

			if (shape.collidable) {
				shape.collidableActive = shape.collidable->isActive();
				shape.hasBounds = tryGetBounds(shape.collidable, shape.min, shape.max);
				collectPolygonLoops(shape.collidable, shape.polygonLoops);
				shape.hasPolygon = !shape.polygonLoops.empty();
			}

			shapeIndexByObject[object] = _debugShapes.size();
			_debugShapes.push_back(shape);
		}

		std::map<GameObject*, CollisionPhase> contactPhasesByObject;
		_debugContacts.reserve(pendingDispatches.size());
		for (const PendingDispatch& pending : pendingDispatches) {
			if (!pending.first || !pending.second) {
				continue;
			}

			updateContactPhase(contactPhasesByObject, pending.first, pending.phase);
			updateContactPhase(contactPhasesByObject, pending.second, pending.phase);

			CollisionDebugContact debugContact;
			debugContact.first = pending.first;
			debugContact.second = pending.second;
			debugContact.phase = pending.phase;
			debugContact.overlapping = pending.overlapping;
			debugContact.normal = pending.normal;
			debugContact.penetrationDepth = pending.penetrationDepth;
			debugContact.timeOfImpact = pending.timeOfImpact;
			debugContact.separation = pending.separation;

			Collidable* firstCollidable = pending.first->getCollidable();
			Collidable* secondCollidable = pending.second->getCollidable();
			debugContact.midpoint = computeContactMidpoint(firstCollidable, secondCollidable, pending.overlapping);
			_debugContacts.push_back(debugContact);
		}

		for (const auto& objectPhase : contactPhasesByObject) {
			auto shapeItr = shapeIndexByObject.find(objectPhase.first);
			if (shapeItr == shapeIndexByObject.end()) {
				continue;
			}

			CollisionDebugShape& shape = _debugShapes[shapeItr->second];
			shape.hasContact = true;
			shape.phase = objectPhase.second;
		}
	}

	for (const PendingDispatch& pending : pendingDispatches) {
		if (!pending.first || !pending.second) {
			continue;
		}

		if (activeObjectSet.find(pending.first) == activeObjectSet.end() ||
			activeObjectSet.find(pending.second) == activeObjectSet.end()) {
			continue;
		}

		Collidable* firstCollidable = pending.first->getCollidable();
		Collidable* secondCollidable = pending.second->getCollidable();

		dispatchPair(
			pending.first,
			pending.second,
			firstCollidable,
			secondCollidable,
			pending.phase,
			pending.overlapping,
			pending.normal,
			pending.penetrationDepth,
			pending.timeOfImpact,
			pending.separation);
	}

	_activePairs.swap(currentPairs);

	for (GameObject* object : activeObjects) {
		if (!object) {
			continue;
		}
		_previousPositions[object] = object->getPosition();
	}

#if _DEBUG
	if (collectDebugData) {
		std::map<std::string, int> layerContactCounts;
		for (const PendingDispatch& pending : pendingDispatches) {
			if (pending.phase == CollisionPhase::Exit) {
				continue;
			}

			Tile* tile = nullptr;
			if (pending.first && pending.first->getType() == GameObject::GAME_OBJ_TILE) {
				tile = (Tile*)pending.first;
			}
			else if (pending.second && pending.second->getType() == GameObject::GAME_OBJ_TILE) {
				tile = (Tile*)pending.second;
			}

			if (!tile) {
				continue;
			}

			std::string layerName = tile->getLayerName();
			if (layerName.empty()) {
				layerName = "(unnamed)";
			}
			layerContactCounts[layerName] += 1;
		}

		static float statsTimer = 0.0f;
		statsTimer += std::max(0.0f, dt);
		if (statsTimer >= 0.5f) {
			statsTimer = 0.0f;
			const TileSet::CollisionLoadStats loadStats = TileSet::getCollisionLoadStats();

			std::string layerContactSummary = "none";
			if (!layerContactCounts.empty()) {
				layerContactSummary.clear();
				int emitted = 0;
				for (const auto& entry : layerContactCounts) {
					if (emitted >= 8) {
						layerContactSummary += ",...";
						break;
					}
					if (!layerContactSummary.empty()) {
						layerContactSummary += ",";
					}
					layerContactSummary += entry.first;
					layerContactSummary += ":";
					layerContactSummary += std::to_string(entry.second);
					++emitted;
				}
			}

			char buffer[768];
			sprintf_s(
				buffer,
				sizeof(buffer),
				"Collision stats: explicit=%d missing=%d concave_decomposed=%d ccd_substeps=%d max_correction=%.3f inferred_starts=%d layer_contacts=%s\n",
				loadStats.explicitColliders,
				loadStats.missingColliders,
				loadStats.decomposedConcavePolygons,
				substeps,
				maxCorrectionMagnitude,
				inferredStartCount,
				layerContactSummary.c_str());
			DEBUG_MSG(buffer);
		}
	}
#endif
}
