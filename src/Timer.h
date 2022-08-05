// File: Timer.h
// Author: Stanley Taveras
// Purpose: A timer for Win32

// TODO: Move to System namespace and make a platform-agnostic timer.
// TODO: Rebase units to milliseconds instead of seconds.

#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <chrono>

class Timer
{
	double			m_dDelta;
	double			m_dFrameCap;

	unsigned int	m_uiTicks;
	unsigned int	m_uiCurrentFPS;

#ifdef _WIN32
	LARGE_INTEGER	m_liFrequency;
	LARGE_INTEGER	m_liDelta;
	LARGE_INTEGER	m_liElapsed;
	LARGE_INTEGER	m_liTimer; // Used internally for calculating FPS
#else
	typedef std::chrono::high_resolution_clock::time_point TimePoint;

	TimePoint m_tpTime;
	TimePoint m_tpElapsed;
	TimePoint m_tpDelta;
#endif

	double _GetTimer(void);
	void _ResetTimer(void) { 
#ifdef _WIN32
		QueryPerformanceCounter(&m_liTimer); 
#endif
	}

public:
	Timer(void);
	~Timer(void) {}

	unsigned int GetFPS(void) const { return m_uiCurrentFPS; }
	double GetDeltaTime(void) const { return m_dDelta; }
	double GetElapsedTime(void) const;

	void LimitFrameRate(unsigned int uiFrameRate);

	void Update(void);
	void ResetElapsed(void);
};