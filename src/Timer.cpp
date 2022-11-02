
#include "Timer.h"

#include <sstream>

// I'm just realizing that this timer has been based on fractional seconds, instead of milliseconds...
// The bugs are going to be hilarious.

using namespace std::chrono;

using ms = duration<double, std::milli>;

Timer::Timer(void) : m_dDelta(0.0f),
					 m_dFrameCap(0.0f),
					 m_uiTicks(0) {
#ifdef _WIN32
	QueryPerformanceFrequency(&m_liFrequency);
	QueryPerformanceCounter(&m_liDelta);
	QueryPerformanceCounter(&m_liElapsed);
	QueryPerformanceCounter(&m_liTimer);
#else
	m_tpTime = high_resolution_clock::now();
#endif
}

#ifndef _WIN32
double _Seconds(void) {
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	return now.tv_sec + now.tv_nsec / 1000000000.0;
}
#endif

double Timer::_Timer(void) const 
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	return (double)(liCurrent.QuadPart - m_liTimer.QuadPart) / (double)m_liFrequency.QuadPart;
#else
	return duration_cast<std::chrono::seconds>(high_resolution_clock::now() - m_tpTime).count();
#endif
	return -1;
}

void Timer::_Tock(void)
{ 
#ifdef _WIN32
	QueryPerformanceCounter(&m_liTimer);
#else
	m_tpTime = std::chrono::high_resolution_clock::now();
#endif
}

void Timer::_Tick(void)
{
	m_uiTicks++;

	if (_Timer() >= 1.0f)
	{
		m_uiCurrentFPS = m_uiTicks;
		m_uiTicks = 0;

		_Tock();
	}
}

double Timer::GetElapsedTime(void) const
{
#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	return (double)(liCurrent.QuadPart - m_liElapsed.QuadPart) / (double)m_liFrequency.QuadPart;
#else
	return m_dElapsed;
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
#else
		sleep((unsigned int)(dSleepTime * 1000));
#endif
		m_dFrameCap = m_dDelta;
	}
}

void Timer::Update(void)
{
	_Tick();

#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	m_dDelta = (double)(liCurrent.QuadPart - m_liDelta.QuadPart) / (double)m_liFrequency.QuadPart;

	m_liDelta = liCurrent;
#else
	m_dDelta = _Seconds() - m_dNow;
	m_dNow = _Seconds();
	m_dElapsed += m_dDelta;
#endif
}

void Timer::Reset(void) { 
#ifdef _WIN32
		QueryPerformanceCounter(&m_liElapsed);
#else
	m_dElapsed = 0.0;
	m_dNow = _Seconds();
#endif
}

#define _CRT_SECURE_NO_WARNINGS 1

std::string Timer::GetTimeStamp(void) const
{
	time_t t = time(0);
	struct tm now;
	//localtime(&t);
	localtime_s(&now, &t);

	std::stringstream ss;
	ss << (now.tm_year + 1900) << '-'
		<< (now.tm_mon + 1) << '-'
		<< now.tm_mday << '_'
		<< now.tm_hour << '-'
		<< now.tm_min << '-'
		<< now.tm_sec;

	return ss.str();
}