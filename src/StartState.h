#pragma once
#include "GameState.h"
class StartState : public GameState
{
	IRenderer::RenderList* _RenderList;
public:
	void OnEnter(void);
	void OnExecute(float time);
	void OnExit(void);
};