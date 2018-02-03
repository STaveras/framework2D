#pragma once
#include "GameState.h"
#include "GameObject.h"
class PlayState : public GameState
{
	Player* _player;
    Animation* _backgroundNoise;
    IRenderer::RenderList* _RenderList;

    enum COLORED_BLOCKS
    {
        RED_BLOCK,
        ORANGE_BLOCK,
        YELLOW_BLOCK,
        GREEN_BLOCK,
        BLUE_BLOCK,
        PURPLE_BLOCK,
        MONO_BLOCK,
        NUM_BLOCKS
    };

    GameObject _Blocks[NUM_BLOCKS];

public:
	void onEnter(void);
	void onExecute(float time);
	void onExit(void);

	virtual void OnKeyPressed(const Event& e);
	virtual void OnKeyReleased(const Event& e);
};