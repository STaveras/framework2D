// File: Engine2D.cpp
// The Engine2D class is the central point of the application
#include "Engine2D.h"
#include "Game.h"
#include "IInput.h"
#include "IRenderer.h"

Engine2D::Engine2D(void) :
	_hasQuit(false),
	_pInput(NULL),
	_pRenderer(NULL)
{}

void Engine2D::initialize(void)
{
	Engine2D::getTimer()->Reset();
	Engine2D::getEventSystem()->initialize(INFINITE);

#ifdef _WIN32
    //DirectInput* pInput = (DirectInput*)Input::CreateDirectInputInterface(rndrWind.GetHWND(), hInstance);
#else
#endif

	// TODO: Separate these sequences to their their own process thread (input thread, game thread, render thread, network thread... etc.)

	if (_pInput)
		_pInput->initialize();

	if (_pRenderer)
	{
		_pRenderer->initialize();
		_pRenderer->SetClearColor(NULL);
	}

	if (_pGame)
		_pGame->Begin();
}

void Engine2D::update(void)
{
	Engine2D::getEventSystem()->processEvents();

	if (_pInput)
		_pInput->update();

	if (_pRenderer)
		_pRenderer->Render();

	if (_pGame)
		_pGame->update(Engine2D::getTimer());

	Engine2D::getTimer()->update();
}

void Engine2D::shutdown(void)
{
	if (_pGame)
		_pGame->End();

	if (_pRenderer)
		_pRenderer->shutdown();

	if (_pInput)
		_pInput->shutdown();

	Engine2D::getEventSystem()->shutdown();
}

IRenderer* Engine2D::getRenderer(void)
{
	Engine2D* engine = Engine2D::getInstance();

	if (engine->_pRenderer)
		return engine->_pRenderer;
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