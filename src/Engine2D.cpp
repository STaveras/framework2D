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
	_Timer.ResetElapsed();
	_EventSystem.Initialize(INFINITE);

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
	_EventSystem.ProcessEvents();

	if (_pInput)
		_pInput->Update();

	if (_pRenderer)
		_pRenderer->Render();

	if (_pGame)
		_pGame->Update(&_Timer);

	_Timer.Update();
}

void Engine2D::Shutdown(void)
{
	if (_pGame)
		_pGame->End();

	if (_pRenderer)
		_pRenderer->Shutdown();

	if (_pInput)
		_pInput->Shutdown();

	_EventSystem.Shutdown();
}
// Stanley Taveras