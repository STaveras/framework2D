// File: Camera.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

// TODO: Remove DirectX functions used to set up matrices at some point to make this more generalized and reusable

#include "Camera.h"

#include "Square.h"

Camera::Camera(void):
GameObject(GAME_OBJ_CAMERA),
m_nScreenWidth(0),
m_nScreenHeight(0),
m_fZoom(1.0f)
{}

void Camera::MoveHorizontally(float amount)
{
	_position.x += amount;
}

void Camera::MoveVertically(float amount)
{
	_position.y += amount;
}

void Camera::Pan(vector2 direction, float amount)
{
	_position += direction * amount;
}

void Camera::Zoom(float amount)
{
	m_fZoom += amount;
	
	if(m_fZoom <= 0.0f)
		m_fZoom = 0.0f;
}

// Trying to make this as simple as possible...
bool Camera::OnScreen(GameObject *object)
{
   Square square(vector2(_position.x - (m_nScreenWidth * 0.5f), 
                         _position.y - (m_nScreenHeight * 0.5f)),
                         (float)m_nScreenWidth, (float)m_nScreenHeight);

   ObjectState *objectState = object->getState();

   if (objectState->getCollidable()) {
      return objectState->getCollidable()->Check(&square);
   }

   return square.Check(object->getPosition());
}