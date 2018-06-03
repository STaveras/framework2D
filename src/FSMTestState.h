// File: FSMTestState.h
// Author: Stanley Taveras
// Created: 3/17/2010
// Modified: 7/4/2010

#pragma once

#include "State.h"
#include "animation.h"

class FSMTestState : public State
{
	Animation* m_pAnimation;

public:
	FSMTestState(void):m_pAnimation(NULL){}

	Animation* GetAnimation(void) const { return m_pAnimation; }
	void SetAnimation(Animation* pAnimation) { m_pAnimation = pAnimation; }

	void onEnter(State* prevState) { if(m_pAnimation){ m_pAnimation->setVisibility(true); m_pAnimation->Play(); } }
	bool onExecute(float fTime) { if(m_pAnimation && !m_pAnimation->IsPlaying()) return false; else return true; } // Let the ::animation manager update animations?
	void onExit(void) { if(m_pAnimation){ m_pAnimation->setVisibility(false); m_pAnimation->Stop(); } }
};