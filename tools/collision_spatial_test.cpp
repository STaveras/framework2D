// Regression comparison for the collision broadphase.  Each scenario runs
// against the legacy map scan and the spatial-query path in lockstep.
#include "../src/CollisionSystem.h"
#include "../src/CollidableGroup.h"
#include "../src/Engine2D.h"
#include "../src/GameObject.h"
#include "../src/ObjectManager.h"
#include "../src/Polygon.h"
#include "../src/Square.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

enum class FixtureKind { Square, Polygon, Compound };

char phaseLetter(CollisionPhase phase)
{
	return phase == CollisionPhase::Enter ? 'E' :
		phase == CollisionPhase::Stay ? 'S' : 'X';
}

struct ContactRecord {
	std::string self;
	std::string other;
	CollisionPhase phase = CollisionPhase::Stay;
	bool overlapping = false;
};

class FixtureObject final : public GameObject
{
	std::string _id;
	FixtureKind _kind;
	Square _square;
	PolygonCollider _polygon;
	CollidableGroup _compound;
	Square _compoundSquare;
	PolygonCollider _compoundPolygon;
	std::vector<ContactRecord> _contacts;

protected:
	void handleCollisionContact(const CollisionContact& contact) override
	{
		if (contact.other) {
			FixtureObject* other = dynamic_cast<FixtureObject*>(contact.other);
			_contacts.push_back(ContactRecord{
				_id,
				other ? other->_id : std::string("<other>"),
				contact.phase,
				contact.overlapping});
		}
	}

public:
	FixtureObject(
		std::string id,
		FixtureKind kind,
		vector2 size,
		const std::vector<vector2>& polygonVertices,
		vector2 position,
		bool isStatic)
		: GameObject(GAME_OBJ_OBJECT),
			_id(std::move(id)),
			_kind(kind),
			_square(vector2(0.0f, 0.0f), size.x, size.y),
			_polygon(),
			_compound(),
			_compoundSquare(vector2(0.0f, 0.0f), size.x * 0.55f, size.y),
			_compoundPolygon()
	{
		if (_kind == FixtureKind::Polygon) {
			_polygon.setLocalVertices(polygonVertices);
		}
		else if (_kind == FixtureKind::Compound) {
			std::vector<vector2> compoundVertices = polygonVertices;
			if (compoundVertices.empty()) {
				compoundVertices = {
					vector2(size.x * 0.45f, 0.0f),
					vector2(size.x, 0.0f),
					vector2(size.x, size.y),
					vector2(size.x * 0.45f, size.y)};
			}
			_compoundPolygon.setLocalVertices(compoundVertices);
			_compound.push_back(&_compoundSquare);
			_compound.push_back(&_compoundPolygon);
		}

		GameObjectState* state = addState("fixture");
		Collidable* collider = _kind == FixtureKind::Square ?
			static_cast<Collidable*>(&_square) :
			(_kind == FixtureKind::Polygon ? static_cast<Collidable*>(&_polygon) :
				static_cast<Collidable*>(&_compound));
		state->setCollidable(collider);
		setStatic(isStatic);
		start();
		setPosition(position);
	}

	const std::string& id() const { return _id; }

	void clearContacts() { _contacts.clear(); }
	const std::vector<ContactRecord>& contacts() const { return _contacts; }

	void setFixtureActive(bool active)
	{
		if (_kind == FixtureKind::Square) {
			_square.setActive(active);
		}
		else if (_kind == FixtureKind::Polygon) {
			_polygon.setActive(active);
		}
		else {
			_compound.setActive(active);
		}
		// Clear GameObject's cloned collider so the state change is observable
		// by both collision paths on the next query.
		setPosition(getPosition());
	}

	void setOneWay()
	{
		SurfaceTraits2D traits;
		traits.flags = SurfaceFlags::Solid | SurfaceFlags::Walkable | SurfaceFlags::OneWay;
		if (_kind == FixtureKind::Square) {
			_square.setSurfaceTraits(traits);
		}
		else if (_kind == FixtureKind::Polygon) {
			_polygon.setSurfaceTraits(traits);
		}
		else {
			_compound.setSurfaceTraits(traits);
		}
		setPosition(getPosition());
	}
};

struct World {
	ObjectManager manager;
	CollisionSystem collision;
	std::vector<std::unique_ptr<FixtureObject>> owned;

	FixtureObject* add(
		const std::string& id,
		FixtureKind kind,
		vector2 size,
		vector2 position,
		bool isStatic,
		const std::vector<vector2>& polygonVertices = {})
	{
		auto object = std::make_unique<FixtureObject>(
			id, kind, size, polygonVertices, position, isStatic);
		FixtureObject* result = object.get();
		owned.push_back(std::move(object));
		manager.addObject(id.c_str(), result);
		return result;
	}

	FixtureObject* get(const std::string& id) const
	{
		for (const auto& object : owned) {
			if (object && object->id() == id) {
				return object.get();
			}
		}
		return nullptr;
	}

	void remove(const std::string& id)
	{
		FixtureObject* object = get(id);
		if (object) {
			manager.removeObject(object);
		}
	}

	void clearContacts()
	{
		for (auto& object : owned) {
			if (object) object->clearContacts();
		}
	}
};

struct TwinWorlds {
	World legacy;
	World spatial;

	template<typename AddOperation>
	explicit TwinWorlds(AddOperation addOperation)
	{
		addOperation(legacy);
		addOperation(spatial);
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

std::vector<std::string> contactSignatures(const World& world)
{
	std::vector<std::string> signatures;
	for (const auto& object : world.owned) {
		if (!object) continue;
		for (const ContactRecord& contact : object->contacts()) {
			std::ostringstream stream;
			stream << contact.self << '>' << contact.other << ':'
				<< phaseLetter(contact.phase) << ':' << (contact.overlapping ? '1' : '0');
			signatures.push_back(stream.str());
		}
	}
	std::sort(signatures.begin(), signatures.end());
	return signatures;
}

bool hasContactPrefix(const std::vector<std::string>& contacts, const std::string& prefix)
{
	return std::any_of(contacts.begin(), contacts.end(), [&](const std::string& value) {
		return value.rfind(prefix, 0) == 0;
	});
}

void compareWorlds(const TwinWorlds& worlds, const std::string& scenario, int tick)
{
	const std::vector<std::string> legacyContacts = contactSignatures(worlds.legacy);
	const std::vector<std::string> spatialContacts = contactSignatures(worlds.spatial);
	if (legacyContacts != spatialContacts) {
		std::ostringstream message;
		message << scenario << " tick " << tick << " contact mismatch\nlegacy: ";
		for (const std::string& value : legacyContacts) message << value << ' ';
		message << "\nspatial: ";
		for (const std::string& value : spatialContacts) message << value << ' ';
		fail(message.str());
	}

	for (const auto& legacyObject : worlds.legacy.owned) {
		if (!legacyObject) continue;
		const FixtureObject* spatialObject = worlds.spatial.get(legacyObject->id());
		if (!spatialObject) {
			// Objects removed from both managers remain owned for pointer safety;
			// compare only active manager entries below.
			continue;
		}
		const bool legacyActive = worlds.legacy.manager.getObjects().find(legacyObject->id()) !=
			worlds.legacy.manager.getObjects().end();
		const bool spatialActive = worlds.spatial.manager.getObjects().find(spatialObject->id()) !=
			worlds.spatial.manager.getObjects().end();
		if (legacyActive != spatialActive) {
			fail(scenario + " tick " + std::to_string(tick) + " manager membership mismatch for " + legacyObject->id());
		}
		if (!legacyActive) continue;

		const vector2 legacyPosition = legacyObject->getPosition();
		const vector2 spatialPosition = spatialObject->getPosition();
		const vector2 legacyVelocity = legacyObject->getVelocity();
		const vector2 spatialVelocity = spatialObject->getVelocity();
		constexpr float kTolerance = 0.001f;
		if (std::fabs(legacyPosition.x - spatialPosition.x) > kTolerance ||
			std::fabs(legacyPosition.y - spatialPosition.y) > kTolerance ||
			std::fabs(legacyVelocity.x - spatialVelocity.x) > kTolerance ||
			std::fabs(legacyVelocity.y - spatialVelocity.y) > kTolerance) {
			std::ostringstream message;
			message << scenario << " tick " << tick << " transform mismatch for " << legacyObject->id();
			fail(message.str());
		}
	}
}

template<typename Operation>
void step(TwinWorlds& worlds, const std::string& scenario, int tick, Operation operation, float dt = 1.0f / 60.0f)
{
	operation(worlds.legacy);
	operation(worlds.spatial);
	worlds.legacy.clearContacts();
	worlds.spatial.clearContacts();
	worlds.legacy.collision.update(worlds.legacy.manager.getObjects(), dt);
	worlds.spatial.collision.update(worlds.spatial.manager, dt);
	compareWorlds(worlds, scenario, tick);
}

template<typename AddOperation>
TwinWorlds makeTwin(AddOperation addOperation)
{
	return TwinWorlds(addOperation);
}

void testFastCcdAndWall()
{
	TwinWorlds worlds = makeTwin([](World& world) {
		world.add("runner", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(0.0f, 20.0f), false);
		world.add("wall", FixtureKind::Square, vector2(10.0f, 100.0f), vector2(50.0f, 0.0f), true);
		world.get("runner")->setVelocity(vector2(240.0f, 0.0f));
	});
	step(worlds, "ccd-wall", 0, [](World&) {}, 0.0f);
	step(worlds, "ccd-wall", 1, [](World& world) {
		world.get("runner")->setPosition(120.0f, 20.0f);
	}, 0.5f);
	step(worlds, "ccd-wall", 2, [](World& world) {
		world.get("runner")->setPosition(130.0f, 20.0f);
	}, 1.0f / 60.0f);
}

void testTouchAndExit()
{
	TwinWorlds worlds = makeTwin([](World& world) {
		world.add("touching", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(0.0f, 0.0f), false);
		world.add("edge", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(10.0f, 0.0f), true);
	});
	step(worlds, "touch-exit", 0, [](World&) {});
	const std::vector<std::string> firstContacts = contactSignatures(worlds.legacy);
	require(std::find(firstContacts.begin(), firstContacts.end(), "touching>edge:E:1") != firstContacts.end(),
		"touch-exit did not report the initial touching Enter");
	step(worlds, "touch-exit", 1, [](World& world) {
		world.get("touching")->setPosition(-20.0f, 0.0f);
	});
	const std::vector<std::string> exitContacts = contactSignatures(worlds.legacy);
	require(std::find(exitContacts.begin(), exitContacts.end(), "touching>edge:X:0") != exitContacts.end(),
		"touch-exit did not report Exit after separating");
}

void testDynamicPairOrder()
{
	// Exercise noncommutative dynamic resolution with a static object present.
	// The map scan and spatial path must produce the same corrected transforms,
	// even when the static object sorts between the dynamic pair's names.
	TwinWorlds worlds = makeTwin([](World& world) {
		world.add("first", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(0.0f, 0.0f), false);
		world.add("second", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(6.0f, 0.0f), false);
		world.add("middle-static", FixtureKind::Square, vector2(6.0f, 30.0f), vector2(-30.0f, -10.0f), true);
	});
	const vector2 firstInitial = worlds.legacy.get("first")->getPosition();
	const vector2 secondInitial = worlds.legacy.get("second")->getPosition();
	step(worlds, "dynamic-order", 0, [](World&) {});
	const vector2 firstCorrected = worlds.legacy.get("first")->getPosition();
	const vector2 secondCorrected = worlds.legacy.get("second")->getPosition();
	require(firstCorrected.x != firstInitial.x || firstCorrected.y != firstInitial.y,
		"dynamic-order did not resolve the first dynamic body");
	require(secondCorrected.x != secondInitial.x || secondCorrected.y != secondInitial.y,
		"dynamic-order did not resolve the second dynamic body");
	step(worlds, "dynamic-order", 1, [](World&) {});
}

void testDynamicTouchContact()
{
	TwinWorlds worlds = makeTwin([](World& world) {
		world.add("touch-first", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(0.0f, 0.0f), false);
		world.add("touch-second", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(10.0f, 0.0f), false);
	});
	step(worlds, "dynamic-touch", 0, [](World&) {});
	const std::vector<std::string> contacts = contactSignatures(worlds.legacy);
	require(std::find(contacts.begin(), contacts.end(), "touch-first>touch-second:E:1") != contacts.end(),
		"dynamic-touch did not dispatch the first-to-second contact");
	require(std::find(contacts.begin(), contacts.end(), "touch-second>touch-first:E:1") != contacts.end(),
		"dynamic-touch did not dispatch the second-to-first contact");
}

void testCorrectionIntoNeighbor(bool neighborEarlier)
{
	TwinWorlds worlds = makeTwin([neighborEarlier](World& world) {
		// The wall pushes the body at x=0 to the right. It starts just short
		// of its adjacent body, so the repeated broadphase query must discover
		// the newly entered pair during this same update.
		const char* pushed = neighborEarlier ? "z" : "a";
		const char* neighbor = neighborEarlier ? "a" : "b";
		world.add(neighbor, FixtureKind::Square, vector2(10.0f, 10.0f), vector2(12.0f, 20.0f), false);
		world.add(pushed, FixtureKind::Square, vector2(10.0f, 10.0f), vector2(0.0f, 20.0f), false);
		world.add("wall", FixtureKind::Square, vector2(10.0f, 20.0f), vector2(-5.0f, 15.0f), true);
	});
	const std::string scenario = neighborEarlier ? "correction-earlier-neighbor" : "correction-later-neighbor";
	step(worlds, scenario, 0, [](World&) {});
	const std::vector<std::string> contacts = contactSignatures(worlds.legacy);
	const std::string pushedToNeighbor = std::string(neighborEarlier ? "z" : "a") + ">" +
		(neighborEarlier ? "a" : "b") + ":E:";
	const std::string neighborToPushed = std::string(neighborEarlier ? "a" : "b") + ">" +
		(neighborEarlier ? "z" : "a") + ":E:";
	require(hasContactPrefix(contacts, pushedToNeighbor) ||
		hasContactPrefix(contacts, neighborToPushed),
		scenario + " did not discover the newly entered dynamic neighbor");
}

void testCompoundCollider()
{
	TwinWorlds worlds = makeTwin([](World& world) {
		world.add("compound", FixtureKind::Compound, vector2(20.0f, 12.0f), vector2(10.0f, 20.0f), false);
		world.add("compound-wall", FixtureKind::Square, vector2(10.0f, 40.0f), vector2(24.0f, 0.0f), true);
	});
	step(worlds, "compound-fallback", 0, [](World&) {});
	const std::vector<std::string> contacts = contactSignatures(worlds.legacy);
	require(hasContactPrefix(contacts, "compound>compound-wall:E:") ||
		hasContactPrefix(contacts, "compound-wall>compound:E:"),
		"compound-fallback did not report the compound collider contact");
	step(worlds, "compound-fallback", 1, [](World& world) {
		world.get("compound-wall")->setPosition(200.0f, 0.0f);
	});
}

void testOneWayCrossing()
{
	TwinWorlds worlds = makeTwin([](World& world) {
		FixtureObject* platform = world.add(
			"platform", FixtureKind::Square, vector2(100.0f, 10.0f), vector2(0.0f, 50.0f), true);
		platform->setOneWay();
		world.add("jumper", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(20.0f, 20.0f), false);
	});
	step(worlds, "one-way", 0, [](World&) {});
	step(worlds, "one-way", 1, [](World& world) {
		world.get("jumper")->setPosition(20.0f, 70.0f);
	});
	step(worlds, "one-way", 2, [](World& world) {
		world.get("jumper")->setPosition(20.0f, 30.0f);
	});
	step(worlds, "one-way", 3, [](World& world) {
		world.get("jumper")->setPosition(20.0f, 70.0f);
	});
}

void testStaticMoveAndRemoval()
{
	TwinWorlds worlds = makeTwin([](World& world) {
		world.add("body", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(20.0f, 20.0f), false);
		world.add("block", FixtureKind::Square, vector2(10.0f, 10.0f), vector2(20.0f, 20.0f), true);
	});
	step(worlds, "static-change", 0, [](World&) {});
	step(worlds, "static-change", 1, [](World& world) {
		world.get("block")->setPosition(200.0f, 200.0f);
	});
	step(worlds, "static-change", 2, [](World& world) {
		world.get("block")->setFixtureActive(false);
	});
	step(worlds, "static-change", 3, [](World& world) {
		world.remove("block");
	});
}

} // namespace

int main()
{
	Engine2D::getEventSystem()->initialize(INFINITE);
	testFastCcdAndWall();
	testTouchAndExit();
	testDynamicPairOrder();
	testCorrectionIntoNeighbor(false);
	testCorrectionIntoNeighbor(true);
	testCompoundCollider();
	testOneWayCrossing();
	testStaticMoveAndRemoval();
	std::cout << "PASS: collision legacy and spatial broadphases agree across dynamic ordering, CCD, touching, neighbor correction, compound colliders, one-way crossing, and static changes\n";
	return 0;
}
