
#include "Timer.h"

Timer::Timer(void) : m_lDelta(0.0f),
					 m_lFrameCap(0.0f),
					 m_uiTicks(0)
{
#ifdef _WIN32
	QueryPerformanceFrequency(&m_liFrequency);
	QueryPerformanceCounter(&m_liDelta);
	QueryPerformanceCounter(&m_liElapsed);
	QueryPerformanceCounter(&m_liTimer);
#else
	m_tpTime = m_tpElapsed = m_tpDelta = std::chrono::high_resolution_clock::now();
#endif
}

long Timer::_GetTimer(void)
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	return (double)(liCurrent.QuadPart - m_liTimer.QuadPart) / (double)m_liFrequency.QuadPart;
#else
	// return std::chrono::high_resolution_clock::now().time_since_epoch().count();
	return std::chrono::duration_cast<std::chrono::duration<long>>(std::chrono::high_resolution_clock::now() - m_tpTime).count();
#endif
	return -1;
}

long Timer::GetElapsedTime(void) const
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	return (double)(liCurrent.QuadPart - m_liElapsed.QuadPart) / (double)m_liFrequency.QuadPart;
#else
	return std::chrono::duration_cast<std::chrono::duration<long>>(std::chrono::high_resolution_clock::now() - m_tpElapsed).count();
#endif
	return -1;
}

void Timer::LimitFrameRate(unsigned int uiFrameRate)
{
	if (!uiFrameRate)
		return;

	double dFrameTime = 1.0 / uiFrameRate;
	double dSleepTime = (m_lFrameCap += dFrameTime) - m_lDelta;

	if (dSleepTime > 0.0)
	{
#ifdef _WIN32
		Sleep((DWORD)(dSleepTime * 1000));
#else
		sleep((unsigned int)(dSleepTime * 1000));
#endif
		m_lFrameCap = m_lDelta;
	}
}

void Timer::Update(void)
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	m_dDelta = (double)(liCurrent.QuadPart - m_liDelta.QuadPart) / (double)m_liFrequency.QuadPart;

	m_liDelta = liCurrent;
#else
	m_tpDelta = TimePoint(std::chrono::high_resolution_clock::now() - m_tpDelta); // something about this is wrong?
	m_lDelta = m_tpDelta.time_since_epoch().count();
#endif
	m_uiTicks++;

	if(_GetTimer() >= 1.0f)
	{
		m_uiCurrentFPS = m_uiTicks;
		m_uiTicks = 0;
		_ResetTimer();
	}
}

void Timer::ResetElapsed(void) { 
#ifdef _WIN32
		QueryPerformanceCounter(&m_liElapsed);
#else
		m_tpElapsed = std::chrono::high_resolution_clock::now();
#endif
}