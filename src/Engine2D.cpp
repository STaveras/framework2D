// File: Engine2D.cpp
// The Engine2D class is the central point of the application
#include "Engine2D.h"
#include "Game.h"
#include "IInput.h"
#include "InputTapeRecorder.h"
#include "IRenderer.h"
#include "RuntimeProfile.h"

#include <algorithm>
#include <cstdio>
#include <ctime>

Engine2D::Engine2D(void) :
	_hasQuit(false),
	_game(NULL),
	_input(NULL),
	_renderer(NULL),
	_deterministicMode(false),
	_fixedDeltaSeconds(1.0 / 60.0),
	_frameAccumulatorSeconds(0.0),
	_simulationTick(0),
	_simulationElapsedSeconds(0.0),
	_renderInterpolation(true)
{}

void Engine2D::setFixedDeltaSeconds(double seconds)
{
	const double clamped = std::max(0.000001, seconds);
	Engine2D::getInstance()->_fixedDeltaSeconds = clamped;
}

void Engine2D::initialize(void)
{
	Engine2D::getTimer()->reset();
	Engine2D::getEventSystem()->initialize(INFINITE);
	Engine2D::getInstance()->_frameAccumulatorSeconds = 0.0;
	Engine2D::getInstance()->_simulationTick = 0;
	Engine2D::getInstance()->_simulationElapsedSeconds = 0.0;
	Engine2D::getInstance()->_interpolation.clear();
	InputTapeRecorder::initializeFromEnvironment();

	if (_input)
		_input->initialize();

	if (_renderer)
	{
		_renderer->initialize();
		_renderer->setClearColor(NULL);
	}

	if (_game)
		_game->begin();
}

void Engine2D::update(void)
{
	Timer* timer = Engine2D::getTimer();
	timer->update();
	const double frameDeltaSeconds = std::max(0.0, timer->getRawDeltaTime());

	Engine2D::getEventSystem()->processEvents();

	constexpr int kMaxSimulationStepsPerFrame = 8;
	const double fixedDelta = std::max(0.000001, _fixedDeltaSeconds);
	const bool runsDeterministicSteps = _game && _deterministicMode;
	if (runsDeterministicSteps) {
		const double maxAccumulator = fixedDelta * (double)kMaxSimulationStepsPerFrame;
		_frameAccumulatorSeconds = std::min(maxAccumulator, _frameAccumulatorSeconds + frameDeltaSeconds);
	}

	// With a fixed step, frames that run no simulation step leave input
	// untouched so a latched key press survives until a tick can see it
	if (_input) {
		if (!runsDeterministicSteps || _frameAccumulatorSeconds + 1e-9 >= fixedDelta)
			_input->update();

		// Application-wide F12: save the current framebuffer as a timestamped
		// PNG next to the running game, in any state or screen.
		if (Keyboard* keyboard = _input->getKeyboard()) {
			if (keyboard->pressed(Key::F12)) {
				char path[128]{ 0 };
				std::tm local{};
				const std::time_t now = std::time(nullptr);
#ifdef _WIN32
				localtime_s(&local, &now);
#else
				localtime_r(&now, &local);
#endif
				std::snprintf(path, sizeof(path), "screenshot_%04d%02d%02d_%02d%02d%02d.png",
					local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
					local.tm_hour, local.tm_min, local.tm_sec);
				if (Renderer::mainWindow && Renderer::mainWindow->saveScreenshot(path)) {
					char message[160]{ 0 };
					std::snprintf(message, sizeof(message), "Screenshot saved: %s\n", path);
					DEBUG_MSG(message);
				}
			}
		}
	}

	const bool interpolate = _deterministicMode && _renderInterpolation && _renderer;
	float interpolationAlpha = 1.0f;

	if (_game) {
		if (_deterministicMode) {
			int simulationSteps = 0;
			while (_frameAccumulatorSeconds + 1e-9 >= fixedDelta && simulationSteps < kMaxSimulationStepsPerFrame) {
				if (interpolate)
					_interpolation.snapshot(_renderer);
				timer->setManualDeltaTime(fixedDelta);
				++_simulationTick;
				_simulationElapsedSeconds += fixedDelta;
				{
					RuntimeProfile::Scope profile(RuntimeProfile::Region::GameTick);
					_game->update(timer);
				}
				timer->clearManualDeltaTime();
				_frameAccumulatorSeconds -= fixedDelta;
				++simulationSteps;
			}
			timer->clearManualDeltaTime();
			interpolationAlpha = (float)std::clamp(_frameAccumulatorSeconds / fixedDelta, 0.0, 1.0);
		}
		else {
			timer->clearManualDeltaTime();
			++_simulationTick;
			_simulationElapsedSeconds += frameDeltaSeconds;
			{
				RuntimeProfile::Scope profile(RuntimeProfile::Region::GameTick);
				_game->update(timer);
			}
		}
	}

	if (_renderer) {
		RuntimeProfile::Scope profile(RuntimeProfile::Region::Render);
		if (interpolate)
			_interpolation.apply(_renderer, interpolationAlpha);
		_renderer->render();
		if (interpolate)
			_interpolation.restore(_renderer);
	}
}

void Engine2D::shutdown(void)
{
	if (_game)
		_game->end();

	if (_renderer)
		_renderer->shutdown();

	if (_input)
		_input->shutdown();

	InputTapeRecorder::shutdown();
	Engine2D::getEventSystem()->shutdown();
}

IRenderer* Engine2D::getRenderer(void)
{
	Engine2D* engine = Engine2D::getInstance();

	if (engine->_renderer)
		return engine->_renderer;
	else {
		// TODO: Return the best suited renderer; send an event somewhere to set up a window for that renderer
	}

	return NULL;
}

EventSystem* Engine2D::getEventSystem(void)
{
	static EventSystem _EventSystem;
	return &_EventSystem;
}

Timer* Engine2D::getTimer(void)
{
	static Timer _Timer;
	return &_Timer;
}
