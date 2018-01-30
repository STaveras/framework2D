#include "StartState.h"

void StartState::OnEnter(void)
{
	GameState::OnEnter();

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

void StartState::OnExecute(float time)
{
	GameState::OnExecute(time);
	Engine2D::GetInstance()->GetTimer()->LimitFrameRate(60);
}

void StartState::OnExit(void)
{
	GameState::OnExit();
}