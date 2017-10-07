// File: Timer.h
// Author: Stanley Taveras
// Purpose: A timer for Win32

#pragma once

#include <windows.h>

class Timer
{
	double			m_dDelta;
	double			m_dFrameCap;

	unsigned int	m_uiTicks;
	unsigned int	m_uiCurrentFPS;

	LARGE_INTEGER	m_liFrequency;
	LARGE_INTEGER	m_liDelta;
	LARGE_INTEGER	m_liElapsed;
	LARGE_INTEGER	m_liTimer; // Used internally for calculating FPS

	double _GetTimer(void);
	void _ResetTimer(void) { QueryPerformanceCounter(&m_liTimer); }

public:
	Timer(void);
	~Timer(void) {}

	unsigned int GetFPS(void) const { return m_uiCurrentFPS; }
	double GetDeltaTime(void) const { return m_dDelta; }
	double GetElapsedTime(void) const;

	void LimitFrameRate(unsigned int uiFrameRate);

	void Update(void);
	void ResetElapsed(void) { QueryPerformanceCounter(&m_liElapsed); }
};