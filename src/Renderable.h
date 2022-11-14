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
	vector2 _offset;
	vector2 _center;
	vector2 _scale;

	float _rotation; // (no. of dimensions - 1)

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
    Renderable(void): m_eType(RENDERABLE_TYPE_NULL), 
		_offset(0.0f, 0.0f),
		_center(0.0f, 0.0f),
		_scale(1.0f, 1.0f),
		_rotation(0.0f) {}

public:
	explicit Renderable(RENDERABLE_TYPE eType):m_eType(eType),
		_offset(0.0f, 0.0f),
		_center(0.0f, 0.0f),
		_scale(1.0f, 1.0f),
		_rotation(0.0f) {
		_appearance._isVisible = true; 
	}
	virtual ~Renderable(void) = default;

	RENDERABLE_TYPE getRenderableType(void) const { return m_eType; }

	virtual vector2 getOffset(void) const  { return _offset; }
	virtual vector2 getCenter(void) const { return _center; }
	virtual vector2 getScale(void) const { return _scale; }
	virtual float getRotation(void) const { return _rotation; }

	virtual void setRotation(float fRotation) { _rotation = fRotation; }
	virtual void setOffset(vector2 offset) { _offset = offset; }
	virtual void setCenter(vector2 center) { _center = center; }
	virtual void setScale(vector2 scale) { _scale = scale; }

	// Appearance properties
	/////////////////////////
	virtual bool isVisible(void) const { return _appearance._isVisible; }
	virtual void setVisibility(bool visible) { _appearance._isVisible = visible; }

	virtual float getAlpha(void) const { return _appearance._alphaOpacity; }
	virtual void setAlpha(float fAlpha) { _appearance._alphaOpacity = (fAlpha <= 0.0f) ? 0 : ((fAlpha >= 1.0f) ? 1.0f : fAlpha); }

	virtual bool isFlashing(void) const { return _appearance._isFlashing; }
	virtual void setFlash(bool isFlashing, Color _flashColor = 0xFFFFFFFF, float fFlashRate = 1.0) { _appearance._isFlashing = isFlashing; _appearance._flashColor = _flashColor; _appearance._flashOpacity = fFlashRate; }
	
	virtual Color getTintColor(void) const { return _appearance._tintColor; }
	virtual void setTint(Color _tintColor) { _appearance._tintColor = _tintColor; }

	virtual void mirror(bool mirrorHorizontally, bool mirrorVertically)
	{
		if (mirrorHorizontally)
		{
			this->_appearance._flipHorizontalAxis = mirrorHorizontally;

			_scale.x = -_scale.x;
			_center.x = -_center.x;
		}
		if (mirrorVertically)
		{
			this->_appearance._flipVerticalAxis = mirrorVertically;

			_scale.y = -_scale.y;
			_center.y = -_center.y;
		}
	}

	virtual void center(void) { _offset = _position; }

	// Flashing and other effects should probably be done in a shader; 
	// but color and other basic properties might be okay here
};

#endif  //_IRENDERABLE_H
