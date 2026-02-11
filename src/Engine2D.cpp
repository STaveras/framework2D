// File: Engine2D.cpp
// The Engine2D class is the central point of the application
#include "Engine2D.h"
#include "Game.h"
#include "IInput.h"
#include "InputTapeRecorder.h"
#include "IRenderer.h"

#include <algorithm>

Engine2D::Engine2D(void) :
	_hasQuit(false),
	_game(NULL),
	_input(NULL),
	_renderer(NULL),
	_deterministicMode(false),
	_fixedDeltaSeconds(1.0 / 60.0),
	_frameAccumulatorSeconds(0.0),
	_simulationTick(0),
	_simulationElapsedSeconds(0.0)
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

	if (_input)
		_input->update();

	if (_game) {
		if (_deterministicMode) {
			constexpr int kMaxSimulationStepsPerFrame = 8;
			const double fixedDelta = std::max(0.000001, _fixedDeltaSeconds);
			const double maxAccumulator = fixedDelta * (double)kMaxSimulationStepsPerFrame;
			_frameAccumulatorSeconds = std::min(maxAccumulator, _frameAccumulatorSeconds + frameDeltaSeconds);

			int simulationSteps = 0;
			while (_frameAccumulatorSeconds + 1e-9 >= fixedDelta && simulationSteps < kMaxSimulationStepsPerFrame) {
				timer->setManualDeltaTime(fixedDelta);
				++_simulationTick;
				_simulationElapsedSeconds += fixedDelta;
				_game->update(timer);
				timer->clearManualDeltaTime();
				_frameAccumulatorSeconds -= fixedDelta;
				++simulationSteps;
			}
			timer->clearManualDeltaTime();
		}
		else {
			timer->clearManualDeltaTime();
			++_simulationTick;
			_simulationElapsedSeconds += frameDeltaSeconds;
			_game->update(timer);
		}
	}

	if (_renderer)
		_renderer->render();
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
