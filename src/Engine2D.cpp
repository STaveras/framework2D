// File: Engine2D.cpp
// The Engine2D class is the central point of the application
#include "Engine2D.h"
#include "Game.h"
#include "IInput.h"
#include "IRenderer.h"

Engine2D::Engine2D(void) :
	_hasQuit(false),
	_input(NULL),
	_renderer(NULL)
{}

void Engine2D::initialize(void)
{
	Engine2D::getTimer()->reset();
	Engine2D::getEventSystem()->initialize(INFINITE);

#ifdef _WIN32
    //DirectInput* pInput = (DirectInput*)Input::CreateDirectInputInterface(rndrWind.GetHWND(), hInstance);
#else
#endif

	// TODO: Separate these sequences to their their own process thread (input thread, game thread, render thread, network thread... etc.)

	if (_input)
		_input->initialize();

	if (_renderer)
	{
		_renderer->initialize();
		_renderer->SetClearColor(NULL);
	}

	if (_game)
		_game->begin();
}

void Engine2D::update(void)
{
	Engine2D::getEventSystem()->processEvents();

	if (_input)
		_input->update();

	if (_renderer)
		_renderer->render();

	if (_game)
		_game->update(Engine2D::getTimer());

	Engine2D::getTimer()->update();
}

void Engine2D::shutdown(void)
{
	if (_game)
		_game->end();

	if (_renderer)
		_renderer->shutdown();

	if (_input)
		_input->shutdown();

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


// Stanley Taveras