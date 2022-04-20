#include "StartState.h"

void StartState::onEnter(void)
{
	GameState::onEnter();

	//Animation* animation = _animationManager.CreateAnimation("BackgroundNoise");
	//animation->SetMode(ANIMATION_MODE_OSCILLATE);
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/0.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/1.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/2.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/3.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/4.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/5.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/6.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/7.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/8.png")));
	//animation->AddFrame(_Sprites.Create(Sprite("./data/images/noise/9.png")));
	//animation->Play();
}

void StartState::onExecute(float time)
{
	GameState::onExecute(time);
	Engine2D::getInstance()->getTimer()->LimitFrameRate(60);
}

void StartState::onExit(void)
{
	GameState::onExit();
}