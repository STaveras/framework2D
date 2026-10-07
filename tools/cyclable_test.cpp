#include "../src/Engine2D.h"
#include "../src/GameObject.h"
#include "../src/ObjectManager.h"
#include "../src/Square.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <type_traits>

namespace {

// GameObject reaches Cyclable through StateMachine and Physical. With a
// virtual base there is exactly one, so this conversion is unambiguous.
static_assert(std::is_convertible<GameObject*, Cyclable*>::value,
	"GameObject must have a single, public Cyclable base");

class Counter final : public GameObject
{
	Square _square;
	int _updateCount = 0;

public:
	Counter() : GameObject(GAME_OBJ_OBJECT), _square(vector2(0.0f, 0.0f), 10.0f, 10.0f)
	{
		addState("idle")->setCollidable(&_square);
		start();
	}

	int updateCount() const { return _updateCount; }

	void update(float time) override
	{
		++_updateCount;
		GameObject::update(time);
	}
};

void require(bool condition, const std::string& message)
{
	if (!condition) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}

void testSingleBase()
{
	Counter object;
	Cyclable* viaGameObject = &object;
	Cyclable* viaStateMachine = static_cast<StateMachine*>(&object);
	Cyclable* viaPhysical = static_cast<Physical*>(&object);
	require(viaGameObject == viaStateMachine && viaStateMachine == viaPhysical,
		"StateMachine and Physical see different Cyclable subobjects");

	viaPhysical->setEnabled(false);
	require(!viaStateMachine->isEnabled(), "enabled flag is not shared between bases");
	viaStateMachine->setEnabled(true);
	require(object.isEnabled(), "enabled flag did not come back on");
}

void testDisabledObjectsSkipUpdate()
{
	ObjectManager manager;
	Counter object;
	manager.addObject("counter", &object);

	manager.update(1.0f / 60.0f);
	require(object.updateCount() == 1, "enabled object was not updated");

	object.setEnabled(false);
	manager.update(1.0f / 60.0f);
	require(object.updateCount() == 1, "disabled object was updated");

	object.setEnabled(true);
	manager.update(1.0f / 60.0f);
	require(object.updateCount() == 2, "re-enabled object was not updated");
}

} // namespace

int main()
{
	Engine2D::getEventSystem()->initialize(INFINITE);
	testSingleBase();
	testDisabledObjectsSkipUpdate();
	std::cout << "PASS: one Cyclable per GameObject, disabled objects skip update\n";
	return 0;
}
