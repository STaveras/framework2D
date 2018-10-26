// File: Animation.cpp
#include "Animation.h"
#include "Frame.h"
#include "Image.h"
Animation::Animation(void):
	Renderable(RENDERABLE_TYPE_ANIMATION),
	m_bForward(true),
	m_bPlaying(false),
	_frameIndex(0),
	m_eMode(ANIMATION_MODE_LOOP),
	m_fSpeed(1.0f),
	m_fTimer(0.0f),
	m_szName("")
{}

Animation::Animation(const char* szName):
	Renderable(RENDERABLE_TYPE_ANIMATION),
	m_bForward(true),
	m_bPlaying(false),
	_frameIndex(0),
	m_eMode(ANIMATION_MODE_LOOP),
	m_fSpeed(1.0f),
	m_fTimer(0.0f),
	m_szName(szName)
{}

void Animation::_AdvanceFrame(void)
{
	if (m_bForward)
	{
		if (_frameIndex == m_Frames.Size() - 1)
			_LastFrame();
		else
			_frameIndex++;
	}
	else
	{
		if (_frameIndex == 0)
			_LastFrame();		
		else
			_frameIndex--;
	}
}

void Animation::_LastFrame(void)
{
	switch(m_eMode)
	{
	case ANIMATION_MODE_ONCE:
		m_bPlaying = false;
		break;

	case ANIMATION_MODE_OSCILLATE:
		m_bForward = !m_bForward;
		_Reset();
		break;

	case ANIMATION_MODE_LOOP:
		_Reset();
		break;
	}
}

void ::Animation::_Reset(void)
{
	if (m_bForward)
		_frameIndex = 0;
	else
		_frameIndex = m_Frames.Size() - 1;
}

void Animation::setPosition(vector2 position)
{
	Factory<Frame>::factory_iterator i = m_Frames.Begin();
	for (; i != m_Frames.End(); i++)
		(*i)->GetSprite()->setPosition(position);
}

void Animation::Mirror(bool bHorizontal, bool bVertical)
{
	Factory<Frame>::factory_iterator i = m_Frames.Begin();
	for (; i != m_Frames.End(); i++)
		(*i)->GetSprite()->Mirror(bHorizontal,bVertical);
}

//Frame* Animation::AddFrame(Sprite* sprite)
//{
//	return AddFrame(sprite, 0.0f);
//}

Frame* Animation::AddFrame(Sprite* sprite, float duration)
{
	Frame* frame = m_Frames.Create();
	frame->SetSprite(sprite);
	frame->SetDuration(duration);
	_Reset();
	return frame;
}

void Animation::RemoveFrame(Frame* frame)
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
	if (!m_bPlaying)
		return false;

	if (m_fTimer < m_Frames[_frameIndex]->GetDuration() && m_Frames[_frameIndex]->GetDuration() > 0)
	{
		m_Frames[_frameIndex]->Update(fTime);
		m_fTimer += fTime * m_fSpeed;
	}
	else
	{
		m_fTimer = 0.0f;
		m_Frames[_frameIndex]->Reset();
		_AdvanceFrame();
	}

	return m_bPlaying;
}

bool Animation::operator==(const Animation& a) const
{
	return (m_eMode == a.m_eMode && m_bForward == a.m_bForward && m_fSpeed == a.m_fSpeed && m_szName == a.m_szName && m_Frames == a.m_Frames);
}

Animation* Animation::loadAnimationsFrom(const char* szFilename)
{
   throw "Animation::loadAnimationFrame unimplemented;";
}

//void Animation::removeFrameData(void)
//{
//   
//}

// Author: Stanley Taveras