#pragma once

#ifndef _COLLISIONSYSTEM_H_
#define _COLLISIONSYSTEM_H_

#include "Types.h"

#include <map>
#include <optional>
#include <set>
#include <string>

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

class CollisionSystem
{
	struct CollisionPairKey
	{
		GameObject* first = nullptr;
		GameObject* second = nullptr;

		bool operator<(const CollisionPairKey& rhs) const;
	};

	std::set<CollisionPairKey> _activePairs;

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
};

#endif // _COLLISIONSYSTEM_H_
