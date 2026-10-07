// ButtonState.h
// Shared down/up/pressed/released tracking for keyboards, mice, gamepads and
// actions. Every device used to keep its own current/previous arrays and
// re-derive the edges; they now poll raw state into a ButtonStateSet.

#pragma once

#include <cstddef>
#include <vector>

namespace ButtonEdge
{
	inline bool pressed(bool isDown, bool wasDown) { return isDown && !wasDown; }
	inline bool released(bool isDown, bool wasDown) { return !isDown && wasDown; }
}

class ButtonStateSet
{
	std::vector<unsigned char> _current;
	std::vector<unsigned char> _previous;
	// Presses seen between polls (e.g. from an event callback). A latched
	// button reads as down for the next frame even if it was already released,
	// so taps shorter than a frame are not lost to polling.
	std::vector<unsigned char> _latch;

	bool _valid(int button) const { return button >= 0 && (size_t)button < _current.size(); }

public:
	explicit ButtonStateSet(size_t count = 0) { resize(count); }

	void resize(size_t count)
	{
		_current.assign(count, 0);
		_previous.assign(count, 0);
		_latch.assign(count, 0);
	}

	size_t size(void) const { return _current.size(); }

	// Start a new frame: the current state becomes the previous one.
	void beginFrame(void) { _previous = _current; }

	// Record the polled level for this frame, consuming any latched press.
	void set(int button, bool isDown)
	{
		if (!_valid(button)) {
			return;
		}
		_current[(size_t)button] = (isDown || _latch[(size_t)button]) ? 1 : 0;
		_latch[(size_t)button] = 0;
	}

	void latchPress(int button)
	{
		if (_valid(button)) {
			_latch[(size_t)button] = 1;
		}
	}

	// Out-of-range buttons read as not down (so up() is true for them).
	bool down(int button) const { return _valid(button) && _current[(size_t)button] != 0; }
	bool up(int button) const { return !down(button); }
	bool wasDown(int button) const { return _valid(button) && _previous[(size_t)button] != 0; }
	bool pressed(int button) const { return ButtonEdge::pressed(down(button), wasDown(button)); }
	bool released(int button) const { return ButtonEdge::released(down(button), wasDown(button)); }
};
