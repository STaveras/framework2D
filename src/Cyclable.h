#pragma once

// Anything the engine starts, ticks and finishes. Inherit it virtually so a
// class that reaches it through several bases (GameObject, via StateMachine
// and Physical) still has one Cyclable and one enabled flag.
class Cyclable
{
	bool _enabled = true;

public:
	virtual ~Cyclable(void) = default;

	// Disabled objects are skipped by the systems that tick them
	// (e.g. ObjectManager::update); start/finish are unaffected.
	bool isEnabled(void) const { return _enabled; }
	void setEnabled(bool enabled) { _enabled = enabled; }

	virtual void start(void) = 0;
	virtual void update(float time) = 0;
	virtual void finish(void) = 0;
};
