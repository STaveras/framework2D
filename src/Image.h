// File: Image.h
#if !defined(_SPRITE_H)
#define _SPRITE_H
#include "Renderable.h"
#include "Types.h"
class IRenderer;
class ITexture;
class Image : public Renderable
{
protected:
	const ITexture* m_pTexture;
	float _rotation;
	vector2 m_Center;
	vector2 m_Scale;
	rect m_SrcRect;

public:
	Image(void);
	Image(ITexture* pImage);
  Image(const Image& image);
	Image(const char* filePath,color clearColor = 0, const rect* srcRect = NULL);
	~Image(void);

	const ITexture* getTexture(void) const { return m_pTexture; }
	float GetRotation(void) const { return _rotation; }
	vector2 GetCenter(void) const { return m_Center; }
	vector2 getScale(void) const { return m_Scale; }
	const rect& GetSourceRect(void) const { return m_SrcRect; }
	vector2 GetRectCenter(void) const;

  float getWidth(void) const;
  float getHeight(void) const;

	void SetTexture(ITexture* pTexture) { m_pTexture = pTexture; }
	void SetRotation(float fRotation) { _rotation = fRotation; }
	void SetCenter(vector2 center) { m_Center = center; }
	void setScale(vector2 scale) { m_Scale = scale; }
	void SetSourceRect(const rect& srcRect) { m_SrcRect = srcRect; }

	void Mirror(bool bHorizontal, bool bVertical);

  void center(void);

	virtual Image* Load(const char* filePath, color clearColor = 0, const rect* srcRect = NULL);
	virtual void Unload(void);
};
#endif  //_SPRITE_H
// Author: Stanley Taveras