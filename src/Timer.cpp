
#include "Timer.h"

#include <sstream>

// I'm just realizing that this timer has been based on fractional seconds, instead of milliseconds...
// The bugs are going to be hilarious.

using namespace std::chrono;

using ms = duration<double, std::milli>;

Timer::Timer(void) : _delta(0.0),
					 _frameCap(0.0),
					 _framesPerSecond(0),
					 _timeMultiplier(1.0),
					 _ticks(0) {
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
	_ticks++;

	if (_Timer() >= 1.0f)
	{
		_framesPerSecond = _ticks;
		_ticks = 0;

		_Tock();
	}
}

double Timer::getElapsedTime(void) const
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

void Timer::limitFrameRate(unsigned int uiFrameRate)
{
	if (!uiFrameRate)
		return;

	double dFrameTime = 1.0 / uiFrameRate;
	double dSleepTime = (_frameCap += dFrameTime) - _delta;

	if (dSleepTime > 0.0)
	{
#ifdef _WIN32
		Sleep((DWORD)(dSleepTime * 1000));
#else
		sleep((unsigned int)(dSleepTime * 1000));
#endif
		_frameCap = _delta;
	}
}

void Timer::update(void)
{
	_Tick();

#ifdef _WIN32
	LARGE_INTEGER liCurrent;
	QueryPerformanceCounter(&liCurrent);

	_delta = (double)(liCurrent.QuadPart - m_liDelta.QuadPart) / (double)m_liFrequency.QuadPart;

	m_liDelta = liCurrent;
#else
	_delta = _Seconds() - m_dNow;
	m_dNow = _Seconds();
	m_dElapsed += _delta;
#endif
}

void Timer::reset(void) { 
#ifdef _WIN32
		QueryPerformanceCounter(&m_liElapsed);
#else
	m_dElapsed = 0.0;
	m_dNow = _Seconds();
#endif
}

#define _CRT_SECURE_NO_WARNINGS 1

std::string Timer::getTimeStamp(void)
{
	time_t t = time(0);
	struct tm now;

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