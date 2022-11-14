// File: Animation.cpp
#include "Animation.h"
#include "Frame.h"
#include "Sprite.h"
Animation::Animation(void):
	Renderable(RENDERABLE_TYPE_ANIMATION),
	m_bForward(true),
	m_bPlaying(false),
	_frameIndex(0),
	m_eMode(eOnce),
	m_fSpeed(1.0f),
	m_fTimer(0.0f),
	m_szName("")
{}

Animation::Animation(const char* szName):
	Renderable(RENDERABLE_TYPE_ANIMATION),
	m_bForward(true),
	m_bPlaying(false),
	_frameIndex(0),
	m_eMode(eOnce),
	m_fSpeed(1.0f),
	m_fTimer(0.0f),
	m_szName(szName)
{}

void Animation::_advanceFrame(void)
{
	if (m_bForward)
	{
		if (_frameIndex == m_Frames.Size() - 1)
			_lastFrame();
		else
			_frameIndex++;
	}
	else
	{
		if (_frameIndex == 0)
			_lastFrame();		
		else
			_frameIndex--;
	}
}

void Animation::_lastFrame(void)
{
	switch(m_eMode)
	{
	case eOnce:
		m_bPlaying = false;
		break;

	case eOscillate:
		m_bForward = !m_bForward;
		_reset();
		break;

	case eLoop:
		_reset();
		break;
	}
}

void ::Animation::_reset(void)
{
	if (m_bForward)
		_frameIndex = 0;
	else
		_frameIndex = (unsigned int)(m_Frames.Size() - 1);
}

void Animation::setPosition(vector2 position)
{
	Factory<Frame>::factory_iterator i = m_Frames.Begin();
	for (; i != m_Frames.End(); i++)
		(*i)->GetSprite()->setPosition(position);
}

void Animation::mirror(bool horizontal, bool vertical)
{
	Renderable::mirror(horizontal, vertical);

	Factory<Frame>::factory_iterator i = m_Frames.Begin();
	for (; i != m_Frames.End(); i++)
		(*i)->GetSprite()->mirror(horizontal, vertical);
}

//Frame* Animation::addFrame(Sprite* sprite)
//{
//	return addFrame(sprite, 0.0f);
//}

Frame* Animation::createFrame(Sprite* sprite, float duration)
{
	Frame* frame = m_Frames.Create();
	frame->SetSprite(sprite);
	frame->SetDuration(duration);
	_reset();
	return frame;
}

void Animation::removeFrame(Frame* frame)
{
	Factory<Frame>::factory_iterator itr = m_Frames.Begin();

	for(; itr != m_Frames.End(); itr++)
	{
		if(frame == (*itr))
		{
			m_Frames.Erase(itr);
			break;
		}
	}
}

bool Animation::update(float fTime)
{
	if (!m_bPlaying || !m_Frames.Size())
		return false;

	if (m_fTimer < m_Frames[_frameIndex]->GetDuration() && m_Frames[_frameIndex]->GetDuration() > 0)
	{
		m_Frames[_frameIndex]->update(fTime);
		m_fTimer += fTime * m_fSpeed;
	}
	else
	{
		m_fTimer = 0.0f;
		m_Frames[_frameIndex]->Reset();
		_advanceFrame();
	}

	return m_bPlaying;
}

bool Animation::operator==(const Animation& a) const
{
	return (m_eMode == a.m_eMode && m_bForward == a.m_bForward && m_fSpeed == a.m_fSpeed && m_szName == a.m_szName && m_Frames == a.m_Frames);
}

// Author: Stanley Taveras