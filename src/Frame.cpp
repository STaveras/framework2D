// File: Frame.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/22/2010

#include "Frame.h"
#include "Image.h"

Frame::Frame(void):
m_fDuration(0.0f),
m_pSprite(NULL)
{}

Frame::Frame(Sprite* sprite, float fDuration):
m_fDuration(fDuration),
m_pSprite(sprite)
{}

void Frame::Update(float fTime)
{
	std::list<Trigger>::iterator itr = m_lsTriggers.begin();
	for(; itr != m_lsTriggers.end(); itr++)
		(*itr)();
}

void Frame::Reset(void)
{
	std::list<Trigger>::iterator itr = m_lsTriggers.begin();

	for(; itr != m_lsTriggers.end(); itr++)
		(*itr).Reset();
}

bool Frame::operator==(const Frame& rhs) const
{
	return (this->m_fDuration == rhs.m_fDuration && this->m_pSprite == rhs.m_pSprite && this->m_lsTriggers == rhs.m_lsTriggers);
}