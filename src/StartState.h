#pragma once
#include "GameState.h"
class StartState : public GameState
{
	IRenderer::RenderList* _RenderList;
public:
	void onEnter(void);
	void onExecute(float time);
	void onExit(void);
};