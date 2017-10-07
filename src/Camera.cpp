// File: Camera.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

// TODO: Remove DirectX functions used to set up matrices at some point to make this more generalized and reusable

#include "Camera.h"

Camera::Camera(void):
GameObject(GAME_OBJ_CAMERA),
m_nScreenWidth(0),
m_nScreenHeight(0),
m_fZoom(1.0f)
{}

void Camera::MoveHorizontally(float amount)
{
	m_Position.x += amount;
}

void Camera::MoveVertically(float amount)
{
	m_Position.y += amount;
}

void Camera::Pan(vector2 direction, float amount)
{
	m_Position += direction * amount;
}

void Camera::Zoom(float amount)
{
	m_fZoom += amount;
	
	if(m_fZoom <= 0.0f)
		m_fZoom = 0.0f;
}

void Camera::Update(float fTime)
{
	// ???
}

