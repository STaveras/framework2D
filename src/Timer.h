// File: Timer.h
// Author: Stanley Taveras
// Purpose: A simple timer class

// TODO: Move to System namespace and make a platform-agnostic timer.
// TODO: Rebase units to milliseconds instead of seconds.

#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <chrono>
#include <string>

class Timer
{
	double _timeMultiplier;

	double			_delta;
	double			_frameCap;

	unsigned int	_ticks;
	unsigned int	_framesPerSecond; // moar like ticks per second m i rite??? :(

private:
	// Internal timer
	double _Timer(void) const;
#ifdef _WIN32
	LARGE_INTEGER	m_liFrequency;
	LARGE_INTEGER	m_liDelta;
	LARGE_INTEGER	m_liElapsed;
	LARGE_INTEGER	m_liTimer; // Used internally for calculating FPS
#else
	std::chrono::high_resolution_clock::time_point m_tpTime;
	
	double m_dNow = 0.0;
	double m_dElapsed = 0.0;

#endif
protected:

	void _Tick(void);
	void _Tock(void);

public:
	Timer(void);
	~Timer(void) {}

	double getTimeMultiplier(void) const { return _timeMultiplier; }
	unsigned int getFPS(void) const { return _framesPerSecond; }
	double getDeltaTime(void) const { return _delta * _timeMultiplier; }
	double getElapsedTime(void) const;

	void limitFrameRate(unsigned int uiFrameRate);

	void setTimeMultiplier(double timeMultiplier) { _timeMultiplier = timeMultiplier; }

	void update(void);
	void reset(void);

	static std::string getTimeStamp(void);
};