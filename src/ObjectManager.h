// File: ObjectManager.h
#if !defined(_OBJECTMANAGER_H)
#define _OBJECTMANAGER_H

#include <list>
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

#include "Event.h"
#include "Types.h"

#define EVT_OBJECT_ADDED "EVT_OBJECT_ADDED"
#define EVT_OBJECT_REMOVED "EVT_OBJECT_REMOVED"
#define EVT_OPERATOR_REMOVED "EVT_OBJ_OP_REMOVED"
#define EVT_OPERATOR_ADDED "EVT_OBJ_OP_ADDED"

class GameObject;
class ObjectOperator;
class SpatialIndex2D;

// Maybe should also be a factory?
class ObjectManager
{
	 std::map<std::string, GameObject*> m_mObjects;
	 std::list<ObjectOperator*> m_lsObjOperators;
	SpatialIndex2D* m_spatialIndex = nullptr;
	mutable std::vector<GameObject*> m_dynamicObjects;
	mutable std::unordered_map<GameObject*, size_t> m_objectOrder;
	mutable bool m_spatialMembershipDirty = true;
	mutable uint64_t m_spatialRevision = 0;

	void rebuildSpatialIndexIfNeeded(void) const;

public:
	ObjectManager(void);
	~ObjectManager(void);
	ObjectManager(const ObjectManager&) = delete;
	ObjectManager& operator=(const ObjectManager&) = delete;

	const std::map<std::string, GameObject*>& getObjects(void) const { return m_mObjects; }

	std::string getObjectName(GameObject* object) const {
		std::map<std::string, GameObject*>::const_iterator itr = m_mObjects.begin();
		for (; itr != m_mObjects.end(); itr++) {
			if (itr->second == object) {
				return itr->first;
			}
		}
		return "";
	}

public:
	void update(float fTime);

	void addObject(const char* name, GameObject* object);
	void removeObject(GameObject* object);
	void removeObject(const char* name);

	void pushOperator(ObjectOperator* objOperation);
	void popOperator(void);
	void clearOperators(void);

	size_t numObjects(void) const { return m_mObjects.size(); }

	GameObject* getGameObject(const char* name) {
		auto itr = m_mObjects.find(name ? name : "");
		return itr == m_mObjects.end() ? nullptr : itr->second;
	}
	const GameObject* getGameObject(const char* name) const {
		auto itr = m_mObjects.find(name ? name : "");
		return itr == m_mObjects.end() ? nullptr : itr->second;
	}

	GameObject* operator[](unsigned int index) {
		std::map<std::string, GameObject*>::iterator itr = m_mObjects.begin();
		for (; itr != m_mObjects.end(); itr++) {
			if (index-- == 0)
				return itr->second;
		}
		return NULL;
	}

	GameObject* operator[](const char* name) {
		return this->getGameObject(name);
	}

	// Clears and fills out with objects whose conservative collider AABB
	// intersects [min, max]. Results follow ObjectManager's lexicographic map
	// order and contain each object at most once. Objects whose collider type
	// has no finite bounds are included conservatively. Dynamic objects are
	// tested against their live bounds; static geometry is queried from a
	// persistent uniform grid.
	void queryBounds(const vector2& min, const vector2& max,
		std::vector<GameObject*>& out, bool staticOnly = false) const;

	const std::vector<GameObject*>& getDynamicObjects(void) const;

	// O(1) membership/rank lookups against the current map snapshot. These
	// let collision iteration validate cached dynamic pointers after callbacks
	// mutate object staticness or manager membership.
	bool contains(GameObject* object) const;
	size_t getSpatialOrder(GameObject* object) const;

	// Call this after editing collider geometry through a pointer/reference
	// that bypasses GameObject's normal lifecycle methods.
	void invalidateSpatialIndex(void);

	void sendEvent(Event::event_key key, void* sender);
};
#endif  //_OBJECTMANAGER_H
// Author: Stanley Taveras
