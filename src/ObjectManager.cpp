// File: ObjectManager.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "ObjectManager.h"
#include "GameObject.h"
#include "ObjectOperator.h"
#include "Engine2D.h"
#include "EventSystem.h"
#include "Kinematics2D.h"
#include "SpatialIndex2D.h"
#include "CollidableGroup.h"

#include <unordered_set>
#include <vector>
#include <string>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <limits>

#if __has_include("RuntimeProfile.h")
#include "RuntimeProfile.h"
#define FRAMEWORK_HAS_RUNTIME_PROFILE 1
#else
#define FRAMEWORK_HAS_RUNTIME_PROFILE 0
#endif

namespace
{
constexpr size_t kMaxIndexedCellsPerObject = 16384;

int cellCoordinate(float value)
{
	const double coordinate = std::floor(static_cast<double>(value) / static_cast<double>(SpatialIndex2D::kCellSize));
	if (coordinate <= static_cast<double>(std::numeric_limits<int>::min())) {
		return std::numeric_limits<int>::min();
	}
	if (coordinate >= static_cast<double>(std::numeric_limits<int>::max())) {
		return std::numeric_limits<int>::max();
	}
	return static_cast<int>(coordinate);
}

bool finiteBounds(vector2 min, vector2 max)
{
	return std::isfinite(min.x) && std::isfinite(min.y) &&
		std::isfinite(max.x) && std::isfinite(max.y) &&
		min.x <= max.x && min.y <= max.y;
}

} // namespace

// Kinematics2D::tryGetBounds intentionally returns the union of the bounds it
// understands. For a broad phase that partial result is unsafe when a group
// also contains an unsupported member, so reject the whole compound and let
// the index use its conservative fallback.
bool SpatialIndex2D::tryGetConservativeBounds(const Collidable* collidable, vector2& outMin, vector2& outMax)
{
	if (!collidable) {
		return false;
	}

	if (collidable->getType() != COL_OBJ_GROUP) {
		if (!Kinematics2D::tryGetBounds(collidable, outMin, outMax) || !finiteBounds(outMin, outMax)) {
			return false;
		}
		if (collidable->getType() == COL_OBJ_POLYGON) {
			// Surface sampling permits slight edge extrapolation (t +/- 0.0001).
			// Include that halo, including its y extent on steep/large polygons.
			const vector2 padding = (outMax - outMin) * 0.0001f + vector2(0.001f, 0.001f);
			outMin = outMin - padding;
			outMax = outMax + padding;
		}
		return true;
	}

	const CollidableGroup* group = (const CollidableGroup*)collidable;
	if (!group || group->empty()) {
		return false;
	}

	bool found = false;
	for (const Collidable* member : *group) {
		// CollidableGroup::collidesWith can still delegate to a child even when
		// that child is marked inactive. Include every non-null child in the
		// broad-phase union; unsupported children force the conservative path.
		if (!member) {
			continue;
		}

		vector2 memberMin(0.0f, 0.0f);
		vector2 memberMax(0.0f, 0.0f);
		if (!SpatialIndex2D::tryGetConservativeBounds(member, memberMin, memberMax)) {
			return false;
		}
		if (!found) {
			outMin = memberMin;
			outMax = memberMax;
			found = true;
		}
		else {
			outMin.x = std::min(outMin.x, memberMin.x);
			outMin.y = std::min(outMin.y, memberMin.y);
			outMax.x = std::max(outMax.x, memberMax.x);
			outMax.y = std::max(outMax.y, memberMax.y);
		}
	}

	return found && finiteBounds(outMin, outMax);
}

namespace
{
bool intersects(vector2 lhsMin, vector2 lhsMax, vector2 rhsMin, vector2 rhsMax)
{
	return lhsMin.x <= rhsMax.x && lhsMax.x >= rhsMin.x &&
		lhsMin.y <= rhsMax.y && lhsMax.y >= rhsMin.y;
}
}

void SpatialIndex2D::rebuild(const std::map<std::string, GameObject*>& objects)
{
#if FRAMEWORK_HAS_RUNTIME_PROFILE
	RuntimeProfile::Scope profileScope(RuntimeProfile::Region::SpatialRebuild);
	RuntimeProfile::count(RuntimeProfile::Counter::SpatialRebuilds);
#endif
	_staticCells.clear();
	_allStatic.clear();
	_unboundedStatic.clear();

	for (const auto& entry : objects) {
		GameObject* object = entry.second;
		if (!object || !object->isStatic()) {
			continue;
		}

		vector2 min(0.0f, 0.0f);
		vector2 max(0.0f, 0.0f);
		Collidable* collidable = object->getCollidable();
		if (!collidable) {
			continue;
		}
		_allStatic.push_back(object);
		const bool hasConservativeBounds = SpatialIndex2D::tryGetConservativeBounds(collidable, min, max);
		if (!hasConservativeBounds) {
			// Unknown/unbounded geometry must remain a candidate to preserve
			// correctness. The collision rules still belong to CollisionSystem.
			_unboundedStatic.push_back(object);
			continue;
		}

		const int minCellX = cellCoordinate(min.x);
		const int maxCellX = cellCoordinate(max.x);
		const int minCellY = cellCoordinate(min.y);
		const int maxCellY = cellCoordinate(max.y);
		const uint64_t spanX = static_cast<uint64_t>(static_cast<int64_t>(maxCellX) - static_cast<int64_t>(minCellX) + 1);
		const uint64_t spanY = static_cast<uint64_t>(static_cast<int64_t>(maxCellY) - static_cast<int64_t>(minCellY) + 1);
		if (spanX == 0 || spanY == 0 || spanX > kMaxIndexedCellsPerObject || spanY > kMaxIndexedCellsPerObject ||
			spanX > (kMaxIndexedCellsPerObject / spanY)) {
			_unboundedStatic.push_back(object);
			continue;
		}

		for (int y = minCellY; y <= maxCellY; ++y) {
			for (int x = minCellX; x <= maxCellX; ++x) {
				_staticCells[Cell{ x, y }].push_back(object);
				if (x == std::numeric_limits<int>::max()) {
					break;
				}
			}
			if (y == std::numeric_limits<int>::max()) {
				break;
			}
		}
	}
}

void SpatialIndex2D::collectStaticCandidates(
	const vector2& min,
	const vector2& max,
	std::unordered_set<GameObject*>& out) const
{
	for (GameObject* object : _unboundedStatic) {
		if (object) {
			out.insert(object);
		}
	}

	if (!finiteBounds(min, max)) {
		for (GameObject* object : _allStatic) {
			if (object) {
				out.insert(object);
			}
		}
		return;
	}

	const int minCellX = cellCoordinate(min.x);
	const int maxCellX = cellCoordinate(max.x);
	const int minCellY = cellCoordinate(min.y);
	const int maxCellY = cellCoordinate(max.y);
	const uint64_t spanX = static_cast<uint64_t>(static_cast<int64_t>(maxCellX) - static_cast<int64_t>(minCellX) + 1);
	const uint64_t spanY = static_cast<uint64_t>(static_cast<int64_t>(maxCellY) - static_cast<int64_t>(minCellY) + 1);
	if (spanX == 0 || spanY == 0 || spanX > kMaxIndexedCellsPerObject || spanY > kMaxIndexedCellsPerObject ||
		spanX > (kMaxIndexedCellsPerObject / spanY)) {
		for (GameObject* object : _allStatic) {
			if (object) {
				out.insert(object);
			}
		}
		return;
	}
	for (int y = minCellY; y <= maxCellY; ++y) {
		for (int x = minCellX; x <= maxCellX; ++x) {
			auto itr = _staticCells.find(Cell{ x, y });
			if (itr != _staticCells.end()) {
				for (GameObject* object : itr->second) {
					if (object) {
						out.insert(object);
					}
				}
			}
			if (x == std::numeric_limits<int>::max()) {
				break;
			}
		}
		if (y == std::numeric_limits<int>::max()) {
			break;
		}
	}
}

ObjectManager::ObjectManager(void)
	: m_spatialIndex(new SpatialIndex2D())
{
}

ObjectManager::~ObjectManager(void)
{
	delete m_spatialIndex;
	m_spatialIndex = nullptr;
}

void ObjectManager::rebuildSpatialIndexIfNeeded(void) const
{
	const uint64_t currentRevision = GameObject::getSpatialIndexRevision();
	if (!m_spatialMembershipDirty && m_spatialRevision == currentRevision) {
		return;
	}

	m_dynamicObjects.clear();
	m_dynamicObjects.reserve(m_mObjects.size());
	m_objectOrder.clear();
	m_objectOrder.reserve(m_mObjects.size());
	size_t objectOrder = 0;
	for (const auto& entry : m_mObjects) {
		GameObject* object = entry.second;
		if (object && m_objectOrder.find(object) == m_objectOrder.end()) {
			m_objectOrder.emplace(object, objectOrder++);
		}
		if (object && !object->isStatic()) {
			m_dynamicObjects.push_back(object);
		}
	}

	m_spatialIndex->rebuild(m_mObjects);
	m_spatialRevision = currentRevision;
	m_spatialMembershipDirty = false;
}

// How we should start thinking about events:
// SYSTEM EVENTS: The backend, mechanical stuff that glues the ''engine'' together (out/in)
// SIMULATION EVENTS: Pertinent only to the objects, and their interactions with one another. (in/out)

void ObjectManager::update(float fTime)
{
	const std::vector<GameObject*> objects = this->getDynamicObjects();
	std::vector<ObjectOperator*> removalList;

	// 1) Update all objects and apply operators.
	for (GameObject* object : objects)
	{
		if (!object || !this->contains(object) || object->isStatic()) {
			continue;
		}

		object->update(fTime);
		if (!this->contains(object) || object->isStatic()) {
			continue;
		}

		for (ObjectOperator* objOperator : m_lsObjOperators) {
			if (objOperator->isEnabled() && !(*objOperator)(object))
				removalList.push_back(objOperator);
		}
	}

	// 2) Remove finished operators once.
	if (!removalList.empty()) {
		std::unordered_set<ObjectOperator*> removed;
		for (ObjectOperator* objOp : removalList) {
			if (!objOp || removed.find(objOp) != removed.end()) {
				continue;
			}
			removed.insert(objOp);
			m_lsObjOperators.remove(objOp);
			Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOp, Event::event_priority_immediate);
		}
	}
}

void ObjectManager::removeObject(GameObject* object)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
	for (; itr != m_mObjects.end(); itr++) {
		if (itr->second == object) {
			if (object) {

				object->finish(); m_mObjects.erase(itr);
				m_spatialMembershipDirty = true;

				Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object, Event::event_priority_immediate); 
				
				break;
			}
		}
	}
}

void ObjectManager::removeObject(const char* name)
{
	GameObject* object = NULL;
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
	for (; itr != m_mObjects.end(); itr++) {
		if (itr->first == name) {
			object = itr->second;

			if (object)
			{
				object->finish(); m_mObjects.erase(itr);
				m_spatialMembershipDirty = true;

				Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_REMOVED, object, Event::event_priority_immediate); 
				
				break;
			}
		}
	}
}

void ObjectManager::addObject(const char* name, GameObject* object)
{
	std::string requestedName = (name && name[0] != '\0') ? std::string(name) : std::string("object");
	std::string resolvedName = requestedName;

	auto existing = m_mObjects.find(resolvedName);
	if (existing != m_mObjects.end() && existing->second != object) {
#if _DEBUG
		char buffer[256];
		sprintf_s(buffer, sizeof(buffer),
			"ObjectManager key collision on '%s' (existing=%p incoming=%p). Auto-suffixing.\n",
			resolvedName.c_str(),
			existing->second,
			object);
		DEBUG_MSG(buffer);
#endif

		unsigned int suffix = 2;
		do {
			resolvedName = requestedName + "#" + std::to_string(suffix++);
			existing = m_mObjects.find(resolvedName);
		} while (existing != m_mObjects.end());
	}

	m_mObjects[resolvedName] = object;
	m_spatialMembershipDirty = true;

	Engine2D::getEventSystem()->sendEvent(EVT_OBJECT_ADDED, object);
}

// NOTE: Maybe use a map instead and add/remove the operators by name?

void ObjectManager::pushOperator(ObjectOperator* objOperation)
{
	m_lsObjOperators.push_back(objOperation);

	Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_ADDED, objOperation);
}

void ObjectManager::popOperator(void)
{
	ObjectOperator* objOperator = m_lsObjOperators.back(); m_lsObjOperators.pop_back();
	Engine2D::getEventSystem()->sendEvent(EVT_OPERATOR_REMOVED, objOperator);
}

void ObjectManager::clearOperators(void)
{
	while (!m_lsObjOperators.empty()) {
		this->popOperator();
	}
}

void ObjectManager::sendEvent(Event::event_key key, void* sender)
{
	std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();

	for (; itr != m_mObjects.end(); itr++) {
		itr->second->sendInput(key, sender);
	}
}

void ObjectManager::queryBounds(
	const vector2& min,
	const vector2& max,
	std::vector<GameObject*>& out,
	bool staticOnly) const
{
#if FRAMEWORK_HAS_RUNTIME_PROFILE
	RuntimeProfile::Scope profileScope(RuntimeProfile::Region::SpatialQuery);
#endif
	rebuildSpatialIndexIfNeeded();
	out.clear();

	vector2 queryMin = min;
	vector2 queryMax = max;
	if (queryMin.x > queryMax.x) {
		std::swap(queryMin.x, queryMax.x);
	}
	if (queryMin.y > queryMax.y) {
		std::swap(queryMin.y, queryMax.y);
	}
	const bool conservativeQuery = !finiteBounds(queryMin, queryMax);

	std::unordered_set<GameObject*> candidates;
	candidates.reserve(32);
	m_spatialIndex->collectStaticCandidates(queryMin, queryMax, candidates);
	if (!staticOnly) {
		for (GameObject* object : m_dynamicObjects) {
			if (!object) {
				continue;
			}

			vector2 objectMin(0.0f, 0.0f);
			vector2 objectMax(0.0f, 0.0f);
			Collidable* collidable = object->getCollidable();
			if (!collidable) {
				continue;
			}
		#if FRAMEWORK_HAS_RUNTIME_PROFILE
			RuntimeProfile::count(RuntimeProfile::Counter::SpatialCandidates);
			#endif
			const bool hasConservativeBounds = SpatialIndex2D::tryGetConservativeBounds(collidable, objectMin, objectMax);
			if (conservativeQuery || !hasConservativeBounds ||
				intersects(objectMin, objectMax, queryMin, queryMax)) {
				candidates.insert(object);
			}
		}
	}

	std::vector<GameObject*> orderedCandidates;
	orderedCandidates.reserve(candidates.size());
	for (GameObject* object : candidates) {
		if (object) {
			orderedCandidates.push_back(object);
		}
	}
	std::sort(orderedCandidates.begin(), orderedCandidates.end(), [this](GameObject* lhs, GameObject* rhs) {
		auto lhsItr = m_objectOrder.find(lhs);
		auto rhsItr = m_objectOrder.find(rhs);
		const size_t lhsOrder = lhsItr == m_objectOrder.end() ? std::numeric_limits<size_t>::max() : lhsItr->second;
		const size_t rhsOrder = rhsItr == m_objectOrder.end() ? std::numeric_limits<size_t>::max() : rhsItr->second;
		return lhsOrder < rhsOrder;
	});

	for (GameObject* object : orderedCandidates) {
		if (staticOnly && !object->isStatic()) {
			continue;
		}
		vector2 objectMin(0.0f, 0.0f);
		vector2 objectMax(0.0f, 0.0f);
		Collidable* collidable = object->getCollidable();
		if (!collidable) {
			continue;
		}
	#if FRAMEWORK_HAS_RUNTIME_PROFILE
		RuntimeProfile::count(RuntimeProfile::Counter::SpatialCandidates);
	#endif
		const bool hasBounds = SpatialIndex2D::tryGetConservativeBounds(collidable, objectMin, objectMax);
		if (conservativeQuery || !hasBounds || intersects(objectMin, objectMax, queryMin, queryMax)) {
			out.push_back(object);
		}
	}
}

const std::vector<GameObject*>& ObjectManager::getDynamicObjects(void) const
{
	rebuildSpatialIndexIfNeeded();
	return m_dynamicObjects;
}

bool ObjectManager::contains(GameObject* object) const
{
	rebuildSpatialIndexIfNeeded();
	return object && m_objectOrder.find(object) != m_objectOrder.end();
}

size_t ObjectManager::getSpatialOrder(GameObject* object) const
{
	rebuildSpatialIndexIfNeeded();
	auto itr = m_objectOrder.find(object);
	return itr == m_objectOrder.end() ? std::numeric_limits<size_t>::max() : itr->second;
}

void ObjectManager::invalidateSpatialIndex(void)
{
	for (const auto& entry : m_mObjects) {
		if (entry.second) {
			entry.second->refreshCollisionGeometry();
		}
	}
	m_spatialMembershipDirty = true;
	GameObject::invalidateSpatialIndex();
}
