// File: Frame.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/1/2010

#if !defined(_FRAME_H)
#define _FRAME_H

#include "Sprite.h"
#include "Trigger.h"
#include "Types.h"
#include <list>

// This is mostly relevant to the Animation class 

class Frame
{
	float _frameDuration;
	Sprite* _sprite;
	Collidable* _collidable;
	std::list<Trigger> _triggers;

protected:
	friend class IRenderer;

public:
	Frame(void);
	Frame(Sprite* pSprite, float fDuration);
	~Frame(void){}

	Sprite* getSprite(void) { return _sprite; }
	void setSprite(Sprite* pSprite) { _sprite = pSprite; }

	Collidable* getCollidable(void) const { return _collidable; }
	void setCollidable(Collidable* collidable) { _collidable = collidable; }

	float getDuration(void) const { return _frameDuration; }
	void setDuration(float fDuration) { _frameDuration = fDuration; }

	void addTrigger(Trigger::TYPE type, std::string value, Trigger::MODE mode = Trigger::ONCE){ _triggers.push_back(Trigger(type,value,mode)); }
	void addTrigger(const Trigger& trigger) { _triggers.push_back(trigger); }
	bool removeTrigger(const Trigger& desc);

	// Goes triggers any associated triggers for this frame...
	void update(float fTime);

	// Resets all triggers for this frame.
	void reset(void);
	
	// Flips sign of scale and center point on the specified axes 
	void mirror(bool bHorizontal, bool bVertical);

	bool operator==(const Frame& rhs) const;
};

#endif  //_FRAME_H
