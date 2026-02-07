#include "CollisionSystem.h"

#include "Collidable.h"
#include "GameObject.h"
#include "Renderable.h"
#include "Square.h"

#include <algorithm>
#include <functional>
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

std::optional<vector2> computeContactMidpoint(const Collidable* a, const Collidable* b, bool overlapping)
{
	if (!a || !b) {
		return std::nullopt;
	}

	if (overlapping) {
		vector2 aMin, aMax, bMin, bMax;
		if (tryGetSquareBounds(a, aMin, aMax) && tryGetSquareBounds(b, bMin, bMax)) {
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
	if (!a || !b || a->getType() != COL_OBJ_SQUARE || b->getType() != COL_OBJ_SQUARE) {
		return;
	}

	const Square* aSquare = (const Square*)a;
	const Square* bSquare = (const Square*)b;

	vector2 aMin = aSquare->getMin();
	vector2 aMax = aSquare->getMax();
	vector2 bMin = bSquare->getMin();
	vector2 bMax = bSquare->getMax();

	float overlapX = std::min(aMax.x, bMax.x) - std::max(aMin.x, bMin.x);
	float overlapY = std::min(aMax.y, bMax.y) - std::max(aMin.y, bMin.y);

	if (overlapX <= 0.0f || overlapY <= 0.0f) {
		return;
	}

	vector2 aCenter = aMin + ((aMax - aMin) * 0.5f);
	vector2 bCenter = bMin + ((bMax - bMin) * 0.5f);
	vector2 delta = bCenter - aCenter;

	if (overlapX < overlapY) {
		normal = vector2((delta.x < 0.0f) ? -1.0f : 1.0f, 0.0f);
		penetrationDepth = overlapX;
	}
	else {
		normal = vector2(0.0f, (delta.y < 0.0f) ? -1.0f : 1.0f);
		penetrationDepth = overlapY;
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
			shape.hasBounds = tryGetSquareBounds(shape.collidable, shape.min, shape.max);
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

			if (!collidable->collidesWith(otherCollidable)) {
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
