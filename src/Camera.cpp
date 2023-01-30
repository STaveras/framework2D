// File: Camera.cpp
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

// TODO: Remove DirectX functions used to set up matrices at some point to make this more generalized and reusable

#include "Camera.h"

#include "Square.h"

Camera::Camera(void):
  GameObject(GAME_OBJ_CAMERA),
   _screenWidth(0),
   _screenHeight(0),
   _zoom(1.0f) {
   
   // TODO: Add different states to the camera so it employs different behaviors
   this->addState("Static");
}

void Camera::moveHorizontally(float amount)
{
	_position.x += amount;
}

void Camera::moveVertically(float amount)
{
	_position.y += amount;
}

void Camera::pan(vector2 direction, float amount)
{
	_position += direction * amount;
}

void Camera::update(float time)
{
   GameObject::update(time);

   _position.x = roundf(_position.x);
   _position.y = roundf(_position.y);
}

// Trying to make this as simple as possible...
bool Camera::onScreen(GameObject *object)
{
   if (Collidable* collidable = object->getCollidable())
   { 
      vector2 min{ _position.x - (_screenWidth * 0.5f),
                   _position.y - (_screenHeight * 0.5f) };

      vector2 max{ min.x + _screenWidth,
                   min.y + _screenHeight };

      Square square(min, max);

      return collidable->collidesWith(&square);
   }

   return false;
}