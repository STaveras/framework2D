// File: Appearance.h
// Author: Stanley Taveras
// Created: 2/20/2010
// Modified: 3/3/2010

#if !defined(_APPEARANCE_H)
#define _APPEARANCE_H

#include "Animation.h"
#include "Renderable.h"
#include "Types.h"

class Image;

class Appearance : public Renderable
{
	bool m_bIsFlashing;
	float m_fAlpha;
	color m_Flash;
	color m_Tint;
	Animation* m_pAnimation;
	std::list<Image*> m_lsSprites;

public:
	Appearance(void);
	~Appearance(void);

	bool IsFlashing(void) const { return m_bIsFlashing; }
	float GetAlpha(void) const { return m_fAlpha; }
	color GetFlashColor(void) const { return m_Flash; }
	color GetTint(void) const { return m_Tint; }
	Animation* GetAnimation(void) { return m_pAnimation; }
	const std::list<Image*>& GetSprites(void) const { return m_lsSprites; }

	void SetIsFlashing(bool bFlash) { m_bIsFlashing = bFlash; }
	void SetAlpha(float fAlpha) { m_fAlpha = fAlpha; }
	void SetFlashColor(color color) { m_Flash = color; }
	void SetTint(color color) { m_Tint = color; }
	void SetAnimation(Animation* pAnimation) { m_pAnimation = pAnimation; }
	void setPosition(vector2 position);

	void PushSprite(Image* sprite) { m_lsSprites.push_back(sprite); }
	void PopSprite(void) { m_lsSprites.pop_back(); }
	void Mirror(bool bHorizontal, bool bVertical);

	void Update(float fTime);
};

#endif  //_APPEARANCE_H