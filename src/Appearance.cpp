// File: Appearance.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/3/2010

#include "Appearance.h"
#include "Animation.h"
#include "Image.h"

Appearance::Appearance(void):
Renderable(RENDERABLE_TYPE_NULL),
m_bIsFlashing(false),
m_fAlpha(1.0f),
m_Flash(0xFFFFFFFF),
m_Tint(0xFFFFFFFF),
m_pAnimation(NULL)
{}

Appearance::~Appearance(void)
{}

void Appearance::SetPosition(vector2 position)
{
	if(m_pAnimation)
		m_pAnimation->SetPosition(position);

	std::list<Image*>::iterator itr = m_lsSprites.begin();

	for(; itr != m_lsSprites.end(); itr++)
	{
		(*itr)->SetPosition(position);
	}
}

void Appearance::Mirror(bool bHorizontal, bool bVertical)
{
	//if(m_pAnimation)
	//	m_pAnimation->Mirror(bHorizontal, bVertical);

	std::list<Image*>::iterator itr = m_lsSprites.begin();

	for(; itr != m_lsSprites.end(); itr++)
	{
		(*itr)->Mirror(bHorizontal, bVertical);
	}
}

void Appearance::Update(float fTime)
{
	//m_pAnimation->Update(fTime);
	// TODO : Implement the flashing functionality
}