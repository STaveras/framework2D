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

void Engine2D::Initialize(void)
{
	Engine2D::getTimer()->ResetElapsed();
	Engine2D::getEventSystem()->Initialize(INFINITE);

#ifdef _WIN32
    //DirectInput* pInput = (DirectInput*)Input::CreateDirectInputInterface(rndrWind.GetHWND(), hInstance);
#else
#endif

	if (_pInput)
		_pInput->Initialize();

	if (_pRenderer)
	{
		_pRenderer->Initialize();
		_pRenderer->SetClearColor(NULL);
	}

	if (_pGame)
		_pGame->Begin();
}

void Engine2D::Update(void)
{
	Engine2D::getEventSystem()->ProcessEvents();

	if (_pInput)
		_pInput->Update();

	if (_pRenderer)
		_pRenderer->Render();

	if (_pGame)
		_pGame->Update(Engine2D::getTimer());

	Engine2D::getTimer()->Update();
}

void Engine2D::Shutdown(void)
{
	if (_pGame)
		_pGame->End();

	if (_pRenderer)
		_pRenderer->Shutdown();

	if (_pInput)
		_pInput->Shutdown();

	Engine2D::getEventSystem()->Shutdown();
}

IRenderer* Engine2D::GetRenderer(void)
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