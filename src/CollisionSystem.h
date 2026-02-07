#pragma once

#ifndef _COLLISIONSYSTEM_H_
#define _COLLISIONSYSTEM_H_

#include "Types.h"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

class GameObject;
class Collidable;

enum class CollisionPhase
{
	Enter,
	Stay,
	Exit
};

struct CollisionContact
{
	GameObject* self = nullptr;
	GameObject* other = nullptr;
	Collidable* selfCollidable = nullptr;
	Collidable* otherCollidable = nullptr;
	CollisionPhase phase = CollisionPhase::Stay;
	bool overlapping = false;
	std::optional<vector2> normal;
	std::optional<float> penetrationDepth;
};

struct CollisionDebugShape
{
	GameObject* object = nullptr;
	Collidable* collidable = nullptr;
	bool collidableActive = false;
	bool hasBounds = false;
	bool hasContact = false;
	CollisionPhase phase = CollisionPhase::Stay;
	vector2 min = vector2(0.0f, 0.0f);
	vector2 max = vector2(0.0f, 0.0f);
	vector2 objectPosition = vector2(0.0f, 0.0f);
	vector2 collisionAnchor = vector2(0.0f, 0.0f);
	vector2 renderableOffset = vector2(0.0f, 0.0f);
	vector2 anchorWithRenderableOffset = vector2(0.0f, 0.0f);
};

struct CollisionDebugContact
{
	GameObject* first = nullptr;
	GameObject* second = nullptr;
	CollisionPhase phase = CollisionPhase::Stay;
	bool overlapping = false;
	std::optional<vector2> normal;
	std::optional<float> penetrationDepth;
	std::optional<vector2> midpoint;
};

class CollisionSystem
{
	struct CollisionPairKey
	{
		GameObject* first = nullptr;
		GameObject* second = nullptr;

		bool operator<(const CollisionPairKey& rhs) const;
	};

	std::set<CollisionPairKey> _activePairs;
	std::vector<CollisionDebugShape> _debugShapes;
	std::vector<CollisionDebugContact> _debugContacts;

	static CollisionPairKey makePairKey(GameObject* a, GameObject* b);
	static void computeGeometryHints(const Collidable* a, const Collidable* b, std::optional<vector2>& normal, std::optional<float>& penetrationDepth);
	void dispatchPair(
		GameObject* a,
		GameObject* b,
		Collidable* aCollidable,
		Collidable* bCollidable,
		CollisionPhase phase,
		bool overlapping,
		const std::optional<vector2>& normalHint = std::nullopt,
		const std::optional<float>& penetrationHint = std::nullopt) const;

public:
	void reset(void);
	void update(const std::map<std::string, GameObject*>& objects);

	const std::vector<CollisionDebugShape>& getDebugShapes(void) const { return _debugShapes; }
	const std::vector<CollisionDebugContact>& getDebugContacts(void) const { return _debugContacts; }
};

#endif // _COLLISIONSYSTEM_H_
