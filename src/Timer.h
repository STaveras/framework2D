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
	double			m_dDelta;
	double			m_dFrameCap;

	unsigned int	m_uiTicks;
	unsigned int	m_uiCurrentFPS;

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

	unsigned int GetFPS(void) const { return m_uiCurrentFPS; }
	double GetDeltaTime(void) const { return m_dDelta; }
	double GetElapsedTime(void) const;

	void LimitFrameRate(unsigned int uiFrameRate);
	std::string GetTimeStamp(void) const;

	void update(void);
	void Reset(void);
};