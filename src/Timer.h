// File: Timer.h
// Author: Stanley Taveras
// Purpose: A timer for Win32

#pragma once

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <chrono>

class Timer
{
	long			m_lDelta;
	long			m_lFrameCap;

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

	long _GetTimer(void);
	void _ResetTimer(void) { 
#ifdef _WIN32
		QueryPerformanceCounter(&m_liTimer); 
#endif
	}

public:
	Timer(void);
	~Timer(void) {}

	unsigned int GetFPS(void) const { return m_uiCurrentFPS; }
	long GetDeltaTime(void) const { return m_lDelta; }
	long GetElapsedTime(void) const;

	void LimitFrameRate(unsigned int uiFrameRate);

	void Update(void);
	void ResetElapsed(void);
};