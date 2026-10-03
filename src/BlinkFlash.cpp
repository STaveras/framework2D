#include "BlinkFlash.h"

#include "GameObject.h"

#include <algorithm>

void BlinkFlash::_apply(GameObject& object, bool enabled) const
{
	for (auto stateIt = object.begin(); stateIt != object.end(); ++stateIt) {
		ObjectState* state = static_cast<ObjectState*>(*stateIt);
		if (Renderable* renderable = state->getRenderable()) {
			renderable->setFlash(enabled, _settings.color, _settings.opacity);
		}
	}
}

void BlinkFlash::start(GameObject& object)
{
	_remaining = _settings.duration;
	_phase = _settings.halfPeriod;
	_on = true;
	_apply(object, true);
}

void BlinkFlash::update(GameObject& object, float time)
{
	if (_remaining <= 0.0f) return;

	_remaining = std::max(0.0f, _remaining - time);
	_phase -= time;
	while (_phase <= 0.0f && _remaining > 0.0f) {
		_on = !_on;
		_phase += _settings.halfPeriod;
	}
	if (_remaining <= 0.0f) {
		_on = false;
		_apply(object, false);
	} else {
		_apply(object, _on);
	}
}

void BlinkFlash::stop(GameObject& object)
{
	_remaining = 0.0f;
	_phase = 0.0f;
	_on = false;
	_apply(object, false);
}
