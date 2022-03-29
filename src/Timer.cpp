
#include "Timer.h"

#include <cfloat>

Timer::Timer(void):
m_dDelta(0.0f),
	m_dFrameCap(0.0f),
	m_uiTicks(0)
{
#ifdef _WIN32
	QueryPerformanceFrequency(&m_liFrequency);
	QueryPerformanceCounter(&m_liDelta);
	QueryPerformanceCounter(&m_liElapsed);
	QueryPerformanceCounter(&m_liTimer);
#endif
}

double Timer::_GetTimer(void)
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	return (double)(liCurrent.QuadPart - m_liTimer.QuadPart) / (double)m_liFrequency.QuadPart;
#endif
	return -1;
}

double Timer::GetElapsedTime(void) const
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	return (double)(liCurrent.QuadPart - m_liElapsed.QuadPart) / (double)m_liFrequency.QuadPart;
#endif
	return -1;
}

void Timer::LimitFrameRate(unsigned int uiFrameRate)
{
	if (!uiFrameRate)
		return;

	double dFrameTime = 1.0 / uiFrameRate;
	double dSleepTime = (m_dFrameCap += dFrameTime) - m_dDelta;

	if (dSleepTime > 0.0)
	{
#ifdef _WIN32
		Sleep((DWORD)(dSleepTime * 1000));
#endif
		m_dFrameCap = m_dDelta;
	}
}

void Timer::Update(void)
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	m_dDelta = (double)(liCurrent.QuadPart - m_liDelta.QuadPart) / (double)m_liFrequency.QuadPart;

	m_liDelta = liCurrent;
#endif
	m_uiTicks++;

	if(_GetTimer() >= 1.0f)
	{
		m_uiCurrentFPS = m_uiTicks;
		m_uiTicks = 0;
		_ResetTimer();
	}
}