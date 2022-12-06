// File: Camera.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_CAMERA_H)
#define _CAMERA_H

#include "GameObject.h"

// TODO: Define frustrum within the camera

class Camera : public GameObject
{
	 int _screenWidth;
	 int _screenHeight;
	 float _zoom; // NOTE: Actually just a scale -- should be the inverse of scale (i.e. 1.0 - zoom)

public:
	 Camera(void);

	 // Out 
	 vector2 getCenter(void) const { return vector2(_screenWidth / 2.0f, _screenHeight / 2.0f); }

	 int getScreenWidth(void) const { return _screenWidth; }
	 int getScreenHeight(void) const { return _screenHeight; }
	 float getZoom(void) const { return _zoom; }

	 void setScreenWidth(unsigned int uiWidth) { _screenWidth = uiWidth; }
	 void setScreenHeight(unsigned int uiHeight) { _screenHeight = uiHeight; }
	 void setZoom(float fZoom) { _zoom = fZoom; }

	 void moveHorizontally(float amount);
	 void moveVertically(float amount);
	 void pan(vector2 direction, float amount);

	 bool onScreen(GameObject* object);
};

#endif  //_CAMERA_H
