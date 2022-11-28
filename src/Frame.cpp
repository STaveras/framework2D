// File: Frame.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/22/2010

#include "Frame.h"
#include "Sprite.h"

Frame::Frame(void):
m_fDuration(0.0f),
m_pSprite(NULL)
{}

Frame::Frame(Sprite* sprite, float fDuration):
m_fDuration(fDuration),
m_pSprite(sprite)
{}

void Frame::update(float fTime)
{
	// If flashing, set image color to what's specified in this frame's data

	std::list<Trigger>::iterator itr = m_lsTriggers.begin();
	for(; itr != m_lsTriggers.end(); itr++)
		(*itr)();
}

void Frame::reset(void)
{
	std::list<Trigger>::iterator itr = m_lsTriggers.begin();

	for(; itr != m_lsTriggers.end(); itr++)
		(*itr).reset();
}

bool Frame::removeTrigger(const Trigger& desc)
{
	std::list<Trigger>::iterator itr = m_lsTriggers.begin();

	for (; itr != m_lsTriggers.end(); itr++) {
		if ((*itr) == desc) {
			m_lsTriggers.erase(itr); return true;
		}
	}

	return false;
}

void Frame::mirror(bool bHorizontal, bool bVertical)
{
	this->getSprite()->mirror(bHorizontal, bVertical);
}

bool Frame::operator==(const Frame& rhs) const
{
	return (this->m_fDuration == rhs.m_fDuration && this->m_pSprite == rhs.m_pSprite && this->m_lsTriggers == rhs.m_lsTriggers);
}