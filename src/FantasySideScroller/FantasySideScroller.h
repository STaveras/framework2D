//// FantasySideScroller.h
//#pragma once
//
//#include "../Game.h"
//
//#include "GamePlayer.h"
//#include "PlayState.h"
//
//class FantasySideScroller : public Game {
//public:
//    FantasySideScroller();
//    ~FantasySideScroller();
//
//    void begin() override;
//    void end() override;
//
//private:
//    PlayState* _playState = nullptr;
//};
//
//extern FantasySideScroller game;

// FantasySideScroller
// An example game using this framework
////////////////////////////////////////

#pragma once

#include "../Game.h"

class PlayState; // Forward declaration

// Game should hold all the managers?
class FantasySideScroller : public Game
{
	PlayState* _playState;

public:
	FantasySideScroller()
		: _playState(nullptr) {
	}
	~FantasySideScroller() { }

	void begin(void);
	void end(void);
};

extern FantasySideScroller game;