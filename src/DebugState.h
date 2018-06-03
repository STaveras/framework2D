// File: DebugState.h
// Author: Stanley Taveras
// Created: 2/28/2010
// Modified: 3/17/2010

#if !defined(_TESTSTATE_H_)
#define _TESTSTATE_H_

#include "IProgramState.h"
#include "AnimationManager.h"
#include "Camera.h"
#include "Debug.h"
#include "EventSystem.h"
#include "Factory.h"
#include "InputManager.h"
#include "IRenderer.h"
#include "SpriteManager.h"
#include "StateMachine.h"
#include "Stage.h"
#include "Types.h"
#include "VirtualGamePad.h"

class DebugState: public IProgramState
{
	bool m_bStaticBG;
	SpriteManager* _pSpriteManager;
	AnimationManager* _pAnimationManager;
	Camera m_Camera;
	EventSystem m_EventSystem;
	InputManager m_InputManager;
	Stage* m_pStage;

	//struct player
	//{
	//	Animation* _avatar;
	//	VirtualGamePad* _pad;
	//	StateMachine* _guts;

	//	class playerState : public State
	//	{
	//		player* _player;
	//		Animation* _animation;

	//		// nething.be
	//		class playerStateMachine : public StateMachine
	//		{
	//			// of tome

	//		};			

	//	protected:
	//	public:
	//		Animation* getAnimationPointer(void) { return _animation; }
	//	};
	//}Player;

public:
	DebugState(void):m_bStaticBG(false),_pSpriteManager(NULL),_pAnimationManager(NULL),m_pStage(NULL){}

	void onEnter(void);
	void onExecute(float fTime);
	void onExit(void);

	// For testing the input manager
	void OnKeyPress(const Event& evt);
	void OnKeyRelease(const Event& evt);
};

#endif