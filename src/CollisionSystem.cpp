#include "CollisionSystem.h"

#include "Collidable.h"
#include "GameObject.h"
#include "Square.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

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
	for (const auto& entry : objects) {
		GameObject* object = entry.second;
		if (!object) {
			continue;
		}

		activeObjects.push_back(object);
		activeObjectSet.insert(object);
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
