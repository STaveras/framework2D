// File: DebugState.cpp
// Author: Stanley Taveras

#include "Debug.h"
#include "DebugState.h"
#include "Engine2D.h"
#include "FSMTestState.h"
#include "IInput.h"
#include "InputEvent.h"
#include "IKeyboard.h"
#include "KEYBOARD_KEYS_DI.h"
#include "Renderer.h"
#include "SDSParser.h"
#include "Timer.h"
#include "Trigger.h"
#include <vector>

void DebugState::onEnter(void)
{
	//if (DEBUGGING)
	//    Debug::Log->write("DebugState::onEnter()");

	//// Event System setup
	///////////////////////////////////////
	//if (DEBUGGING)
	//    Debug::Log->getInstance()->write("  DebugState: Event System Initializing...");
	//if (DEBUGGING)
	//    Debug::Log->write("  DebugState: Registering Events...");
	//m_EventSystem.RegisterCallback<StateMachine>("EVT_KEYPRESSED", &m_StateMachine, &StateMachine::OnEvent);
	//m_EventSystem.RegisterCallback<StateMachine>("EVT_KEYRELEASED", &m_StateMachine, &StateMachine::OnEvent);

	//if (DEBUGGING)
	//{
	//    Debug::Log->write("  DebugState: Events Registered.");
	//    Debug::Log->getInstance()->write("  DebugState: Event System Initialized.");
	//}

	//// Input System setup
	///////////////////////////////////////
	//m_InputManager.Initialize(&m_EventSystem, Engine2D::getInstance()->GetInput());
	//m_pGamepad = m_InputManager.CreateGamePad();

	//VirtualButton btn("LP");
	//btn.Assign(KBK_A);

	//m_pGamepad->addButton(btn);

	// stuff setup
	/////////////////////////////////////
	IRenderer* pRenderer = Engine2D::getInstance()->GetRenderer();

	pRenderer->isBackgroundStatic(false);
	pRenderer->SetCamera(&m_Camera);

	m_InputManager.Initialize(&m_EventSystem, Engine2D::getInstance()->GetInput());

	m_EventSystem.Initialize(INFINITE);

	m_EventSystem.RegisterCallback<DebugState>("EVT_KEYPRESSED", this, &DebugState::OnKeyPress);
	m_EventSystem.RegisterCallback<DebugState>("EVT_KEYRELEASED", this, &DebugState::OnKeyRelease);
	
	_pSpriteManager = new SpriteManager();
	_pAnimationManager = new AnimationManager();

	//Animation* animation = _pAnimationManager->CreateAnimation("CHARACTER_GUY_IDLING");
	//animation->AddFrame(_pSpriteManager->CreateSprite("./data/characters/black/sprites/idling/0.bmp",0xFFFF00FF), 0.33f);
	//animation->AddFrame(_pSpriteManager->CreateSprite("./data/characters/black/sprites/idling/1.bmp",0xFFFF00FF), 0.33f);
	//animation->Play();
	//_pAnimationManager->AddFrame(animation, "./data/characters/the guy/sprites/idling/0.bmp", 0.033f, 0xFFFF00FF);7

	//m_Camera.SetZoom(2.0);
	//m_Camera.setPosition(0, -112);
	// Load assets
	/////////////////////////////////////
	//Engine2D::getInstance()->GetRenderer()->SetClearColor(0xFFFFFFFF);
	//Animation* pAnimation = m_AnimationManager.CreateAnimation("BG_NOISE");
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/0.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/1.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/2.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/3.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/4.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/5.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/6.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/7.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/8.png", NULL), 0.073f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/sprites/noise/9.png", NULL), 0.073f));
	//pAnimation->setPosition(-192,-112);

	//pAnimation->Play();

	//Renderer::AddToRenderList(pAnimation);

	//pAnimation = m_AnimationManager.CreateAnimation("CHR_BLACK_IDLE");
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/characters/black/sprites/idle/0.bmp", NULL, 0xffff00ff), 0.5f));
	//pAnimation->AddFrame(Frame(m_SpriteManager.CreateSprite("./data/characters/black/sprites/idle/1.bmp", NULL, 0xffff00ff), 0.5f));
	//(*pAnimation)[0].GetSprite()->SetCenter(vector2(5,23));
	//(*pAnimation)[1].GetSprite()->SetCenter(vector2(5,22));

	//pAnimation->Play();

	//Renderer::AddToRenderList(pAnimation);
	/////////////////////////////////////
	//TheGuy._pad = m_InputManager.CreateGamePad();

	//VirtualButton btn("attack");
	//btn.Assign(KBK_A);

	//TheGuy._pad->addButton(btn);

	// States
	///////////
	//player::playerState* CHR_GUY_IDLING = TheGuy._guts.GetStateFactory().CreateDerived<player::playerState>(State("CHR_GUY_IDLING"));	
	// Are people angels or demons? Both? Neither?
	// Everyone's beep is different.
}

//void DebugState::player::

void DebugState::onExecute(float fTime)
{
#pragma region SystemUpdates
	m_EventSystem.ProcessEvents();
	m_InputManager.Update(fTime);
	
	if (_pAnimationManager)
		_pAnimationManager->Update(fTime);
	//TheGuy._guts.update(fTime);
#pragma endregion
	// Input
#pragma region Camera
	if (Debug::Mode.isEnabled())
	{
		float fMoveSpeed = 50.0f * fTime;
		float fZoomSpeed = 10.0f * fTime;

		IInput* pInput = Engine2D::getInstance()->GetInput();
		IKeyboard* pKeyboard = pInput->GetKeyboard();

		if(pKeyboard->KeyDown(KBK_NUMPAD4))
			m_Camera.MoveHorizontally(-fMoveSpeed);
		else if(pKeyboard->KeyDown(KBK_NUMPAD6))
			m_Camera.MoveHorizontally(fMoveSpeed);

		if(pKeyboard->KeyDown(KBK_NUMPAD8))
			m_Camera.MoveVertically(-fMoveSpeed);
		else if(pKeyboard->KeyDown(KBK_NUMPAD2))
			m_Camera.MoveVertically(fMoveSpeed);

		if (pKeyboard->KeyPressed(KBK_NUMPAD5))
			m_Camera.setPosition(vector2(0,0));

		if(pKeyboard->KeyDown(KBK_ADD))
			m_Camera.Zoom(fZoomSpeed);
		else if(pKeyboard->KeyDown(KBK_SUBTRACT))
			m_Camera.Zoom(-fZoomSpeed);
		else if(pKeyboard->KeyDown(KBK_NUMPADENTER))
			m_Camera.SetZoom(1.0);

		if(pKeyboard->KeyDown(KBK_PGUP))
			m_Camera.SetRotation(m_Camera.GetRotation()-0.05f);
		else if(pKeyboard->KeyDown(KBK_PGDN))
			m_Camera.SetRotation(m_Camera.GetRotation()+0.05f);
		else if(pKeyboard->KeyPressed(KBK_HOME))
			m_Camera.SetRotation(0.0f);

		if(pKeyboard->KeyPressed(KBK_F6))
			m_pStage->setVisibility(!m_pStage->isVisible());

		if(pKeyboard->KeyPressed(KBK_F7))
			m_bStaticBG = !m_bStaticBG;
	}
#pragma endregion
}

void DebugState::onExit(void)
{
	m_EventSystem.Shutdown();
	
	if (_pAnimationManager)
		delete _pAnimationManager;

	if (_pSpriteManager)
		delete _pSpriteManager;

	if (m_pStage)
		delete m_pStage;
}

void DebugState::OnKeyPress(const Event& evt)
{
	InputEvent& inputEvent = (InputEvent&)evt;
	std::string keyEvent = inputEvent.GetButtonID();
	keyEvent += "_pressed";
	//TheGuy._guts.SendInput(keyEvent.c_str());
	//m_EventSystem.sendEvent<StateMachineEvent>(StateMachineEvent(inputEvent.GetButtonID().c_str(), this));
}

void DebugState::OnKeyRelease(const Event& evt)
{
	InputEvent& inputEvent = (InputEvent&)evt;
	std::string keyEvent = inputEvent.GetButtonID();
	keyEvent += "_released";
	//TheGuy._guts.SendInput(keyEvent.c_str());
}