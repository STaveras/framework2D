// File: Renderable.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/28/2010

#if !defined(_IRENDERABLE_H)
#define _IRENDERABLE_H

#include "color.h"
#include "positionable.h"
#include <list>
#include <math.h>

enum RENDERABLE_TYPE { RENDERABLE_TYPE_NULL, RENDERABLE_TYPE_ANIMATION, RENDERABLE_TYPE_IMAGE, RENDERABLE_TYPE_FONT };

class Renderable : public Positionable
{
	RENDERABLE_TYPE m_eType;
	struct
	{
		bool _isVisible;
		bool m_bIsFlashing;
		float m_fAlpha;
		float m_fFlash;
		color m_Flash;
		color m_Tint;
	}_appearance;

protected:
    Renderable(void):m_eType(RENDERABLE_TYPE_NULL){memset(&_appearance,0,(sizeof(bool)*2+sizeof(float)*2+sizeof(color)*2));}

public:
	explicit Renderable(RENDERABLE_TYPE eType):m_eType(eType){_appearance._isVisible = true;}
  virtual ~Renderable(void) = 0 {}

	bool IsVisible(void) const { return _appearance._isVisible; }
	RENDERABLE_TYPE getRenderableType(void) const { return m_eType; }

	void setVisibility(bool bVisible) { _appearance._isVisible = bVisible; }

	// Appearance properties
	/////////////////////////
	void SetAlpha(float fAlpha) { _appearance.m_fAlpha = (fAlpha <= 0.0f) ? 0 : ((fAlpha >= 1.0f) ? 1.0f : fAlpha); }
	void SetFlash(bool isFlashing, color flashColor, float fFlashRate) { _appearance.m_bIsFlashing = isFlashing; _appearance.m_Flash = flashColor; _appearance.m_fFlash = fFlashRate; }
	void SetTint(color tintColor) { _appearance.m_Tint = tintColor; }
};

#endif  //_IRENDERABLE_H
