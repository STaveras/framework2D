#include "../src/Engine2D.h"
#include "../src/GameObject.h"
#include "../src/ObjectManager.h"
#include "../src/RuntimeProfile.h"
#include "../src/SpatialIndex2D.h"
#include "../src/Square.h"
#include "../src/CollidableGroup.h"
#include "../src/Plane.h"
#include "../src/Polygon.h"
#include "../src/Kinematics2D.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class Fixture final : public GameObject
{
	Square _square;
	Plane _unknown; // Cloneable geometry without a finite broad-phase AABB.
	CollidableGroup _group;
	bool _compound = false;
	std::function<void()> _updateCallback;
	int _updateCount = 0;

public:
	Fixture(const char* name, vector2 position, bool isStatic, bool compound = false)
		: GameObject(GAME_OBJ_OBJECT), _square(vector2(0.0f, 0.0f), 10.0f, 10.0f), _compound(compound)
	{
		GameObjectState* state = addState(name);
		if (_compound) {
			_group.push_back(&_square);
			_group.push_back(&_unknown);
			state->setCollidable(&_group);
		}
		else {
			state->setCollidable(&_square);
		}
		setStatic(isStatic);
		start();
		setPosition(position);
	}

	Square& shape() { return _square; }
	void setUpdateCallback(std::function<void()> callback) { _updateCallback = std::move(callback); }
	int updateCount() const { return _updateCount; }

	void update(float time) override
	{
		++_updateCount;
		if (_updateCallback) {
			_updateCallback();
		}
		GameObject::update(time);
	}
};

[[noreturn]] void fail(const std::string& message)
{
	std::cerr << "FAIL: " << message << '\n';
	std::exit(1);
}

void require(bool condition, const std::string& message)
{
	if (!condition) fail(message);
}

std::vector<std::string> names(const std::vector<GameObject*>& objects, const ObjectManager& manager)
{
	std::vector<std::string> result;
	for (GameObject* object : objects) {
		result.push_back(manager.getObjectName(object));
	}
	return result;
}

void testBoundsAndOrder()
{
	ObjectManager manager;
	Fixture zulu("zulu", vector2(0.0f, 0.0f), true);
	Fixture alpha("alpha", vector2(5.0f, 0.0f), true);
	Fixture farAway("far", vector2(10000.0f, 10000.0f), true);
	manager.addObject("zulu", &zulu);
	manager.addObject("alpha", &alpha);
	manager.addObject("far", &farAway);

	std::vector<GameObject*> result;
	manager.queryBounds(vector2(-1.0f, -1.0f), vector2(20.0f, 20.0f), result, true);
	require(names(result, manager) == std::vector<std::string>({"alpha", "zulu"}),
		"static query did not use lexical map order or deduplicate cell hits");

	manager.queryBounds(vector2(-6.0f, -1.0f), vector2(2.0f, 2.0f), result, true);
	require(names(result, manager) == std::vector<std::string>({"zulu"}),
		"static query returned an object outside its actual bounds");

	manager.queryBounds(vector2(-140.0f, -140.0f), vector2(-120.0f, -120.0f), result, true);
	require(result.empty(), "negative-coordinate query returned a far object");
}

void testLifecycleAndDynamicBounds()
{
	ObjectManager manager;
	Fixture block("block", vector2(0.0f, 0.0f), true);
	Fixture mover("mover", vector2(100.0f, 100.0f), false);
	Fixture decor("decor", vector2(0.0f, 0.0f), true);
	decor.getState()->setCollidable(nullptr);
	manager.addObject("block", &block);
	manager.addObject("mover", &mover);
	manager.addObject("decor", &decor);

	std::vector<GameObject*> result;
	manager.queryBounds(vector2(-2.0f, -2.0f), vector2(12.0f, 12.0f), result);
	require(names(result, manager) == std::vector<std::string>({"block"}),
		"null-collider decoration was returned or dynamic bounds were stale");

	mover.setPosition(vector2(0.0f, 0.0f));
	manager.queryBounds(vector2(-2.0f, -2.0f), vector2(12.0f, 12.0f), result);
	require(names(result, manager) == std::vector<std::string>({"block", "mover"}),
		"dynamic query did not use live bounds");

	block.setPosition(vector2(200.0f, 200.0f));
	manager.queryBounds(vector2(-2.0f, -2.0f), vector2(12.0f, 12.0f), result);
	require(names(result, manager) == std::vector<std::string>({"mover"}),
		"static move did not invalidate the grid");

	block.shape().setWidth(100.0f);
	manager.invalidateSpatialIndex();
	manager.queryBounds(vector2(200.0f, 200.0f), vector2(305.0f, 205.0f), result, true);
	require(names(result, manager) == std::vector<std::string>({"block"}),
		"explicit geometry refresh did not clear the cloned collider");

	block.setStatic(false);
	manager.queryBounds(vector2(200.0f, 200.0f), vector2(305.0f, 205.0f), result, true);
	require(result.empty(), "staticOnly query retained an object after staticness changed");
}

void testConservativeAndHugeQueries()
{
	ObjectManager manager;
	Fixture compound("compound", vector2(0.0f, 0.0f), true, true);
	Fixture plain("plain", vector2(100.0f, 100.0f), true);
	manager.addObject("compound", &compound);
	manager.addObject("plain", &plain);

	vector2 min(0.0f, 0.0f), max(0.0f, 0.0f);
	require(!SpatialIndex2D::tryGetConservativeBounds(compound.getCollidable(), min, max),
		"unsupported compound was treated as a partial finite bound");

	std::vector<GameObject*> result;
	manager.queryBounds(vector2(-1.0f, -1.0f), vector2(1.0f, 1.0f), result, true);
	require(names(result, manager) == std::vector<std::string>({"compound"}),
		"unsupported compound was not retained conservatively");

	manager.queryBounds(vector2(
		-std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity()), vector2(
		std::numeric_limits<float>::infinity(),
		std::numeric_limits<float>::infinity()), result, true);
	require(names(result, manager) == std::vector<std::string>({"compound", "plain"}),
		"nonfinite query did not use conservative fallback");
}

void testUpdateSnapshotMutation()
{
	ObjectManager manager;
	Fixture remover("a-remover", vector2(0.0f, 0.0f), false);
	Fixture victim("b-victim", vector2(0.0f, 0.0f), false);
	manager.addObject("a-remover", &remover);
	manager.addObject("b-victim", &victim);
	remover.setUpdateCallback([&]() {
		manager.removeObject(&victim);
		std::vector<GameObject*> query;
		manager.queryBounds(vector2(-2.0f, -2.0f), vector2(12.0f, 12.0f), query);
	});

	manager.update(0.0f);
	require(remover.updateCount() == 1, "snapshot updater did not run");
	require(victim.updateCount() == 0,
		"ObjectManager dereferenced a removed object from its cached dynamic snapshot");
}

void testLocalCandidateCount()
{
	ObjectManager manager;
	Fixture localFixture("near", vector2(0.0f, 0.0f), true);
	manager.addObject("near", &localFixture);
	std::vector<std::unique_ptr<Fixture>> farObjects;
	for (int i = 0; i < 5000; ++i) {
		const std::string name = "far" + std::to_string(i);
		auto object = std::make_unique<Fixture>(name.c_str(), vector2(100000.0f + i * 20.0f, 100000.0f), true);
		manager.addObject(name.c_str(), object.get());
		farObjects.push_back(std::move(object));
	}

	RuntimeProfile::active = true;
	RuntimeProfile::counters.fill(0);
	RuntimeProfile::samples.fill(RuntimeProfile::Sample{});
	std::vector<GameObject*> result;
	manager.queryBounds(vector2(-2.0f, -2.0f), vector2(12.0f, 12.0f), result, true);
	const std::uint64_t firstRebuilds = RuntimeProfile::counters[static_cast<size_t>(RuntimeProfile::Counter::SpatialRebuilds)];
	const std::uint64_t firstCandidates = RuntimeProfile::counters[static_cast<size_t>(RuntimeProfile::Counter::SpatialCandidates)];
	manager.queryBounds(vector2(-2.0f, -2.0f), vector2(12.0f, 12.0f), result, true);
	const std::uint64_t secondRebuilds = RuntimeProfile::counters[static_cast<size_t>(RuntimeProfile::Counter::SpatialRebuilds)];
	RuntimeProfile::active = false;
	require(firstRebuilds == 1 && secondRebuilds == firstRebuilds,
		"steady spatial queries rebuilt the static grid");
	require(firstCandidates <= 1, "local query visited far static objects");
}

void testPolygonSamplingHalo()
{
	ObjectManager manager;
	PolygonCollider polygon;
	polygon.setLocalVertices({vector2(0, 0), vector2(1, 10000), vector2(0, 10000)});
	GameObject object(GameObject::GAME_OBJ_OBJECT);
	object.addState("slope")->setCollidable(&polygon);
	object.setStatic(true);
	object.start();
	manager.addObject("slope", &object);
	float supportY = 0;
	const float x = -0.00005f;
	require(Kinematics2D::sampleSupportY(object.getCollidable(), x, supportY), "slope edge must sample");
	std::vector<GameObject*> result;
	manager.queryBounds(vector2(x, supportY - 0.001f), vector2(x, supportY + 0.001f), result);
	require(result.size() == 1 && result.front() == &object, "broad phase lost extrapolated slope support");
}

} // namespace

int main()
{
	Engine2D::getEventSystem()->initialize(INFINITE);
	testBoundsAndOrder();
	testLifecycleAndDynamicBounds();
	testConservativeAndHugeQueries();
	testUpdateSnapshotMutation();
	testLocalCandidateCount();
	testPolygonSamplingHalo();
	std::cout << "PASS: spatial bounds, lifecycle invalidation, conservative fallback, and local candidate visits\n";
	return 0;
}
