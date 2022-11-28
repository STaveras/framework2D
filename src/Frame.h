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
	float m_fDuration;
	Sprite* m_pSprite;
	std::list<Trigger> m_lsTriggers;

protected:
	friend class IRenderer;

public:
	Frame(void);
	Frame(Sprite* pSprite, float fDuration);
	~Frame(void){}

	Sprite* getSprite(void) { return m_pSprite; }
	void setSprite(Sprite* pSprite) { m_pSprite = pSprite; }

	float getDuration(void) const { return m_fDuration; }
	void setDuration(float fDuration) { m_fDuration = fDuration; }

	void addTrigger(Trigger::TYPE type, std::string value, Trigger::MODE mode = Trigger::ONCE){ m_lsTriggers.push_back(Trigger(type,value,mode)); }
	void addTrigger(const Trigger& trigger) { m_lsTriggers.push_back(trigger); }
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
