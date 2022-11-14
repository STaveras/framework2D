// File: Renderable.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 2/28/2010

#if !defined(_IRENDERABLE_H)
#define _IRENDERABLE_H

#include "Positionable.h"

#include <list>
#include <math.h>

enum RENDERABLE_TYPE { 
	RENDERABLE_TYPE_NULL, 
	RENDERABLE_TYPE_SPRITE, 
	RENDERABLE_TYPE_ANIMATION,
	RENDERABLE_TYPE_FONT
};

class Renderable: public Positionable
{
public:
	RENDERABLE_TYPE m_eType;

	struct {
		float _alphaOpacity; // to be like, extra verbose as to what it does :|
		float _flashOpacity;
		Color _flashColor;
		Color _tintColor;
		bool _isVisible;
		bool _isFlashing;
		bool _flipVerticalAxis;
		bool _flipHorizontalAxis;
	}_appearance{};

protected:
    Renderable(void): m_eType(RENDERABLE_TYPE_NULL) {}

public:
	explicit Renderable(RENDERABLE_TYPE eType):m_eType(eType) { _appearance._isVisible = true; }
	virtual ~Renderable(void) = default;

	bool IsVisible(void) const { return _appearance._isVisible; }
	RENDERABLE_TYPE getRenderableType(void) const { return m_eType; }

	void setVisibility(bool bVisible) { _appearance._isVisible = bVisible; }

	virtual void mirror(bool mirrorHorizontally, bool mirrorVertically)
	{
		this->_appearance._flipHorizontalAxis = mirrorHorizontally;
		this->_appearance._flipVerticalAxis = mirrorVertically;
	}

	virtual void center(void) {}

	// Appearance properties
	/////////////////////////
	void SetAlpha(float fAlpha) { _appearance._alphaOpacity = (fAlpha <= 0.0f) ? 0 : ((fAlpha >= 1.0f) ? 1.0f : fAlpha); }
	void SetFlash(bool isFlashing, Color _flashColor, float fFlashRate) { _appearance._isFlashing = isFlashing; _appearance._flashColor = _flashColor; _appearance._flashOpacity = fFlashRate; }
	void SetTint(Color _tintColor) { _appearance._tintColor = _tintColor; }
	// Flashing and other effects should probably be done in a shader; 
	// but color and other basic properties might be okay here
};

#endif  //_IRENDERABLE_H
