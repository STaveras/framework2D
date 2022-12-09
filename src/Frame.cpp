// File: Frame.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/22/2010

#include "Frame.h"
#include "Sprite.h"

Frame::Frame(void):
_frameDuration(0.0f),
_sprite(NULL)
{}

Frame::Frame(Sprite* sprite, float fDuration):
_frameDuration(fDuration),
_sprite(sprite)
{}

void Frame::update(float fTime)
{
	// If flashing, set image color to what's specified in this frame's data

	std::list<Trigger>::iterator itr = _triggers.begin();
	for(; itr != _triggers.end(); itr++)
		(*itr)();
}

void Frame::reset(void)
{
	std::list<Trigger>::iterator itr = _triggers.begin();

	for(; itr != _triggers.end(); itr++)
		(*itr).reset();
}

bool Frame::removeTrigger(const Trigger& desc)
{
	std::list<Trigger>::iterator itr = _triggers.begin();

	for (; itr != _triggers.end(); itr++) {
		if ((*itr) == desc) {
			_triggers.erase(itr); return true;
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
	return (this->_frameDuration == rhs._frameDuration && this->_sprite == rhs._sprite && this->_triggers == rhs._triggers);
}