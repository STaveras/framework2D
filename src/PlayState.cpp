#include "PlayState.h"
#include "GameObject.h"
#include "Game.h"
#include "InputEvent.h"
void PlayState::OnEnter(void)
{
	//GameState::OnEnter();
	Engine2D* engine = Engine2D::GetInstance();
	engine->GetEventSystem()->RegisterCallback<PlayState>("EVT_KEYPRESSED", this, &PlayState::OnKeyPressed);
	engine->GetEventSystem()->RegisterCallback<PlayState>("EVT_KEYRELEASED", this, &PlayState::OnKeyReleased);

    _RenderList = engine->GetRenderer()->CreateRenderList();

	//_animationManager.Initialize();
	//_inputManager.Initialize(engine->GetEventSystem(), engine->GetInput());

	_backgroundNoise = new Animation();
	_backgroundNoise->SetMode(ANIMATION_MODE_OSCILLATE);
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/0.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/1.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/2.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/3.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/4.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/5.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/6.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/7.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/8.png")));
	//_backgroundNoise->AddFrame(_Sprites.Create(Sprite("./data/images/noise/9.png")));
	_backgroundNoise->Play();

    _RenderList->push_back(_backgroundNoise);

	//_player = Engine2D::GetGame()->GetPlayer(0);

	// Setting up its controller
	//_player->SetGamePad(_inputManager.CreateGamePad());
	_player->GetGamePad()->AddButton(VirtualButton("ATK",KBK_A));

	// Setting up its controllable object
	_player->SetGameObject(new GameObject());
	//_objectManager.AddObject("Person",_player->GetGameObject());

	// Adding states to define that object's behavior...
	_player->GetGameObject()->AddState("IDLING");
	//Animation* theGuyIdling = _animationManager.CreateAnimation("{ERSON_IDLING");
	//theGuyIdling->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/idling/0.bmp",0xFFFF00FF)),0.333f);
	//theGuyIdling->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/idling/1.bmp",0xFFFF00FF)),0.333f);
	//_player->GetGameObject()->SetAnimation(theGuyIdling);

	_player->GetGameObject()->AddState("ATTACKING");
	//Animation* theGuyAttacking = _animationManager.CreateAnimation("PERSON_ATTACKING");
	//theGuyAttacking->SetMode(ANIMATION_MODE_ONCE);
	//theGuyAttacking->SetVisibility(false);
	//theGuyAttacking->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/attacking/0.bmp",0xFFFF00FF)),0.0167f);
	//theGuyAttacking->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/attacking/1.bmp",0xFFFF00FF)),0.0167f);
	//_player->GetGameObject()->SetAnimation(theGuyAttacking);

	_player->GetGameObject()->RegisterTransition("IDLING","ATK_PRESSED","ATTACKING");
	_player->GetGameObject()->RegisterTransition("ATTACKING","ATK_PRESSED","ATTACKING");
	_player->GetGameObject()->RegisterTransition("ATTACKING","ANIMATION_STOPPED","IDLING");

	_player->GetGameObject()->Initialize();   

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        RECT blockRect = {i*16,0,(i+1)*16,16};        
        //_Blocks[i].AddState("BEING")->SetRenderable(_Sprites.Create(Sprite("./data/images/blocks.png", 0xFFFF00FF, &blockRect)));
    }
    
    // Add 24 mono(grey) blocks to the bottom of the 
    for (int i = 0; i < 24; i++)
    {
        //_Blocks[MONO_BLOCK]
        //_RenderList.push_back(
    }
}

void PlayState::OnExecute(float time)
{
	GameState::OnExecute(time);	
	Engine2D::GetInstance()->GetTimer()->LimitFrameRate(60);
    _backgroundNoise->Update(time);
}

void PlayState::OnExit(void)
{
	//_objectManager.RemoveObject("Person");

	delete _player->GetGameObject();
	_player->SetGameObject(NULL);

	//_inputManager.DestroyGamePad(_player->GetGamePad());
	_player->SetGamePad(NULL);

    delete _backgroundNoise;
    
    Engine2D::GetInstance()->GetRenderer()->DestroyRenderList(_RenderList);

	//_inputManager.Shutdown();
	//_animationManager.Shutdown();
	//_Sprites.Clear();

	//GameState::OnExit();
}

void PlayState::OnKeyPressed(const Event& e)
{
	InputEvent* inputEvent = (InputEvent*)&e;
	std::string stateMachineInput = inputEvent->GetButtonID() + "_PRESSED";
	_player->GetGameObject()->SendInput(stateMachineInput.c_str(),e.GetSender());
}

void PlayState::OnKeyReleased(const Event& e)
{
	InputEvent* inputEvent = (InputEvent*)&e;
	std::string stateMachineInput = inputEvent->GetButtonID() + "_RELEASED";
	_player->GetGameObject()->SendInput(stateMachineInput.c_str(),e.GetSender());
}
