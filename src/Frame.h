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
// Make frame a sprite?
class Frame
{
	float m_fDuration;
	Sprite* m_pSprite;
	std::list<Trigger> m_lsTriggers;

public:
	Frame(void);
	Frame(Sprite* pSprite, float fDuration);
	~Frame(void){}

	float GetDuration(void) const { return m_fDuration; }
	Sprite* GetSprite(void) { return m_pSprite; }

	void SetDuration(float fDuration) { m_fDuration = fDuration; }
	void SetSprite(Sprite* pSprite) { m_pSprite = pSprite; }
	void setPosition(vector2 position) { if(m_pSprite) m_pSprite->setPosition(position); }

	void AddTrigger(Trigger::TYPE type, std::string value, Trigger::MODE mode = Trigger::ONCE){ m_lsTriggers.push_back(Trigger(type,value,mode)); }
	void AddTrigger(const Trigger& trigger) { m_lsTriggers.push_back(trigger); }
	bool RemoveTrigger(const Trigger& desc);

	// Goes triggers any associated triggers for this frame...
	void Update(float fTime);

	// Resets all triggers for this frame.
	void Reset(void);

	bool operator==(const Frame& rhs) const;
};

#endif  //_FRAME_H
