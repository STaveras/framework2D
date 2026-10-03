// BlinkFlash.h
// Blinks a flash tint on and off over every state renderable of a GameObject
// for a fixed time (e.g. a damage flash).

#pragma once

#include "Types.h"

class GameObject;

class BlinkFlash
{
public:
	struct Settings
	{
		float duration = 0.64f;   // Total blink time in seconds
		float halfPeriod = 0.08f; // Seconds between on/off toggles
		float opacity = 0.85f;    // Flash blend over the tint while on
		Color color = 0xFFFF5555;
	};

	BlinkFlash(void) = default;
	explicit BlinkFlash(const Settings& settings) : _settings(settings) {}

	const Settings& getSettings(void) const { return _settings; }
	bool isActive(void) const { return _remaining > 0.0f; }

	// Start (or restart) blinking, beginning in the "on" phase.
	void start(GameObject& object);
	// Advance by time seconds; turns the flash off when the duration runs out.
	void update(GameObject& object, float time);
	// Stop immediately and clear the flash.
	void stop(GameObject& object);

private:
	void _apply(GameObject& object, bool enabled) const;

	Settings _settings;
	float _remaining = 0.0f;
	float _phase = 0.0f;
	bool _on = false;
};
