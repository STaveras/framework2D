// File: Camera.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/20/2010

#if !defined(_CAMERA_H)
#define _CAMERA_H

#include "GameObject.h"

class Camera : public GameObject
{
	int m_nScreenWidth;
	int m_nScreenHeight;
	float m_fZoom;

public:
	Camera(void);

	int GetScreenWidth(void) const { return m_nScreenWidth; }
	int GetScreenHeight(void) const { return m_nScreenHeight; }
	float GetZoom(void) const { return m_fZoom; }
	vector2 GetCenter(void) const { return vector2(m_nScreenWidth/2.0f, m_nScreenHeight/2.0f); }

	void SetScreenWidth(unsigned int uiWidth) { m_nScreenWidth = uiWidth; }
	void SetScreenHeight(unsigned int uiHeight) { m_nScreenHeight = uiHeight; }
	void setZoom(float fZoom) { m_fZoom = fZoom; }

	void MoveHorizontally(float amount);
	void MoveVertically(float amount);
	void Pan(vector2 direction, float amount);
	void Zoom(float amount);

  bool OnScreen(GameObject *object);
};

#endif  //_CAMERA_H
