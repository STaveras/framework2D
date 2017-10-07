// File: Square.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#include "Square.h"
#include "Plane.h"

bool Square::WithCircle(const Circle* circle)
{
	return false;
}

bool Square::WithPlane(const Plane* plane)
{
	//vector2 position = ((m_Position + m_Max) / 2);

	//vector2 toSquare = position - plane->GetPosition();

	//vector2 width = (m_Max - m_Position);

	//// TODO : Remove DirectX-specific code
	//float fRadius = D3DXVec2Length(&width) / 2;
	//float fDotResult = D3DXVec2Dot(&toSquare, &plane->GetNormal());

	//if(fDotResult < fRadius)
	//	return true;

	return false;
}

bool Square::WithSquare(const Square* square)
{
	if(m_Position > square->m_Max || m_Max < square->m_Position)
		return false;
	else
		return true;
}

