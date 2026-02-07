#include "CollisionSystem.h"

#include "Collidable.h"
#include "CollidableGroup.h"
#include "GameObject.h"
#include "Polygon.h"
#include "Renderable.h"
#include "Square.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <utility>
#include <vector>

namespace {
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

	constexpr float axisEpsilon = 0.0001f;
	float minOverlap = std::numeric_limits<float>::max();
	vector2 bestAxis(0.0f, 0.0f);
	bool hasAxis = false;

	auto testAxes = [&](const std::vector<vector2>& vertices) -> bool {
		for (size_t i = 0; i < vertices.size(); ++i) {
			const vector2& p0 = vertices[i];
			const vector2& p1 = vertices[(i + 1) % vertices.size()];
			vector2 edge = p1 - p0;
			if (edge.norm() <= axisEpsilon) {
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
	const std::optional<float>& penetrationHint) const
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
	a->onCollisionContact(contactA);

	CollisionContact contactB;
	contactB.self = b;
	contactB.other = a;
	contactB.selfCollidable = bCollidable;
	contactB.otherCollidable = aCollidable;
	contactB.phase = phase;
	contactB.overlapping = overlapping;
	if (normal) {
		contactB.normal = vector2(-normal->x, -normal->y);
	}
	contactB.penetrationDepth = penetrationDepth;
	b->onCollisionContact(contactB);
}

void CollisionSystem::reset(void)
{
	_activePairs.clear();
	_debugShapes.clear();
	_debugContacts.clear();
}

void CollisionSystem::update(const std::map<std::string, GameObject*>& objects)
{
	struct PendingDispatch
	{
		GameObject* first = nullptr;
		GameObject* second = nullptr;
		CollisionPhase phase = CollisionPhase::Stay;
		bool overlapping = false;
		std::optional<vector2> normal;
		std::optional<float> penetrationDepth;
	};

	std::vector<GameObject*> activeObjects;
	activeObjects.reserve(objects.size());

	std::set<GameObject*> activeObjectSet;
	_debugShapes.clear();
	_debugContacts.clear();

	for (const auto& entry : objects) {
		GameObject* object = entry.second;
		if (!object) {
			continue;
		}

		activeObjects.push_back(object);
		activeObjectSet.insert(object);
	}

	_debugShapes.reserve(activeObjects.size());
	std::map<GameObject*, size_t> shapeIndexByObject;
	for (GameObject* object : activeObjects) {
		if (!object) {
			continue;
		}

		CollisionDebugShape shape;
		shape.object = object;
		shape.objectPosition = object->getPosition();
		shape.collisionAnchor = object->getCollisionAnchor();

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

	for (auto itr = _activePairs.begin(); itr != _activePairs.end();) {
		if (activeObjectSet.find(itr->first) == activeObjectSet.end() ||
			activeObjectSet.find(itr->second) == activeObjectSet.end()) {
			itr = _activePairs.erase(itr);
		}
		else {
			++itr;
		}
	}

	std::set<CollisionPairKey> currentPairs;
	std::vector<PendingDispatch> pendingDispatches;

	for (size_t i = 0; i < activeObjects.size(); ++i) {
		GameObject* object = activeObjects[i];
		if (!object) {
			continue;
		}

		Collidable* collidable = object->getCollidable();
		if (!collidable || !collidable->isActive()) {
			continue;
		}

		for (size_t j = i + 1; j < activeObjects.size(); ++j) {
			GameObject* otherObject = activeObjects[j];
			if (!otherObject) {
				continue;
			}

			if (!object->shouldCollideWith(*otherObject) || !otherObject->shouldCollideWith(*object)) {
				continue;
			}

			Collidable* otherCollidable = otherObject->getCollidable();
			if (!otherCollidable || !otherCollidable->isActive()) {
				continue;
			}

				const bool overlapping = collidable->collidesWith(otherCollidable) || otherCollidable->collidesWith(collidable);
				if (!overlapping) {
					continue;
				}

			CollisionPairKey key = makePairKey(object, otherObject);
			currentPairs.insert(key);

			CollisionPhase phase = (_activePairs.find(key) == _activePairs.end()) ? CollisionPhase::Enter : CollisionPhase::Stay;
			PendingDispatch pending;
			pending.first = object;
			pending.second = otherObject;
			pending.phase = phase;
				pending.overlapping = true;
			computeGeometryHints(collidable, otherCollidable, pending.normal, pending.penetrationDepth);
			pendingDispatches.push_back(pending);
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
			pending.penetrationDepth);
	}

	_activePairs.swap(currentPairs);
}
