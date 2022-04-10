#include "PlayState.h"
#include "GameObject.h"
#include "Game.h"
#include "InputEvent.h"
void PlayState::onEnter(void)
{
	//GameState::onEnter();
	Engine2D* engine = Engine2D::getInstance();
	engine->getEventSystem()->RegisterCallback<PlayState>("EVT_KEYPRESSED", this, &PlayState::OnKeyPressed);
	engine->getEventSystem()->RegisterCallback<PlayState>("EVT_KEYRELEASED", this, &PlayState::OnKeyReleased);

    _RenderList = engine->GetRenderer()->CreateRenderList();

	//_animationManager.Initialize();
	//_inputManager.Initialize(engine->getEventSystem(), engine->GetInput());

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
	//_player->setGamePad(_inputManager.CreateGamePad());
	_player->getGamePad()->addButton(VirtualButton("ATK",KBK_A));

	// Setting up its controllable object
	_player->setGameObject(new GameObject());
	//_objectManager.addObject("Person",_player->getGameObject());

	// Adding states to define that object's behavior...
	_player->getGameObject()->addState("IDLING");
	//Animation* theGuyIdling = _animationManager.CreateAnimation("{ERSON_IDLING");
	//theGuyIdling->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/idling/0.bmp",0xFFFF00FF)),0.333f);
	//theGuyIdling->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/idling/1.bmp",0xFFFF00FF)),0.333f);
	//_player->getGameObject()->setAnimation(theGuyIdling);

	_player->getGameObject()->addState("ATTACKING");
	//Animation* theGuyAttacking = _animationManager.CreateAnimation("PERSON_ATTACKING");
	//theGuyAttacking->SetMode(ANIMATION_MODE_ONCE);
	//theGuyAttacking->setVisibility(false);
	//theGuyAttacking->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/attacking/0.bmp",0xFFFF00FF)),0.0167f);
	//theGuyAttacking->AddFrame(_Sprites.Create(Sprite("./data/characters/person/sprites/attacking/1.bmp",0xFFFF00FF)),0.0167f);
	//_player->getGameObject()->setAnimation(theGuyAttacking);

	_player->getGameObject()->registerTransition("IDLING","ATK_PRESSED","ATTACKING");
	_player->getGameObject()->registerTransition("ATTACKING","ATK_PRESSED","ATTACKING");
	_player->getGameObject()->registerTransition("ATTACKING","ANIMATION_STOPPED","IDLING");

	_player->getGameObject()->initialize();   

    for (int i = 0; i < NUM_BLOCKS; i++)
    {
        RECT blockRect = {i*16,0,(i+1)*16,16};        
        //_Blocks[i].addState("BEING")->SetRenderable(_Sprites.Create(Sprite("./data/images/blocks.png", 0xFFFF00FF, &blockRect)));
    }
    
    // Add 24 mono(grey) blocks to the bottom of the 
    for (int i = 0; i < 24; i++)
    {
        //_Blocks[MONO_BLOCK]
        //_RenderList.push_back(
    }
}

void PlayState::onExecute(float time)
{
	GameState::onExecute(time);	
	Engine2D::getInstance()->GetTimer()->LimitFrameRate(60);
    _backgroundNoise->update(time);
}

void PlayState::onExit(void)
{
	//_objectManager.removeObject("Person");

	delete _player->getGameObject();
	_player->setGameObject(NULL);

	//_inputManager.DestroyGamePad(_player->getGamePad());
	_player->setGamePad(NULL);

    delete _backgroundNoise;
    
    Engine2D::getInstance()->GetRenderer()->DestroyRenderList(_RenderList);

	//_inputManager.shutdown();
	//_animationManager.shutdown();
	//_Sprites.Clear();

	//GameState::onExit();
}

void PlayState::OnKeyPressed(const Event& e)
{
	InputEvent* inputEvent = (InputEvent*)&e;
	std::string stateMachineInput = inputEvent->GetButtonID() + "_PRESSED";
	_player->getGameObject()->SendInput(stateMachineInput.c_str(),e.GetSender());
}

void PlayState::OnKeyReleased(const Event& e)
{
	InputEvent* inputEvent = (InputEvent*)&e;
	std::string stateMachineInput = inputEvent->GetButtonID() + "_RELEASED";
	_player->getGameObject()->SendInput(stateMachineInput.c_str(),e.GetSender());
}
