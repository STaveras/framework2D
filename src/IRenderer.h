// File: IRenderer.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/28/2010

#if !defined(_IRENDERER_H_)
#define _IRENDERER_H_

#include "Factory.h"
#include "ITexture.h"
#include "color.h"
#include <list>

class Camera;

class IRenderer
{
public:
	typedef std::list<class Renderable *> RenderList;

protected:
	bool m_bStaticBG;
	bool m_bFullScreen;
	bool m_bVerticalSync;
	int m_nWidth;
	int m_nHeight;
	color m_ClearColor;
	Camera *m_pCamera;
	Factory<ITexture> m_Textures;
	Factory<RenderList> _RenderLists;

	void _BackgroundColorShift(void);
	ITexture *_TextureExists(const char *szFilename);

public:
	IRenderer(void) : m_bStaticBG(false),
					  m_bFullScreen(false),
					  m_bVerticalSync(false),
					  m_nWidth(0), m_nHeight(0),
					  m_ClearColor(0xFFFFFFFF),
					  m_pCamera(NULL) {
		_RenderLists.Create();
	} // Comes with one global render list

	IRenderer(int nWidth, int nHeight) : m_bStaticBG(false), m_nWidth(nWidth), m_nHeight(nHeight), m_ClearColor(0xFFFFFFFF), m_pCamera(NULL) {}
	virtual ~IRenderer() = 0;

	bool isBackgroundStatic(void) const { return m_bStaticBG; }
	int GetWidth(void) const { return m_nWidth; }
	int GetHeight(void) const { return m_nHeight; }
	color GetClearColor(void) const { return m_ClearColor; }
	Camera *GetCamera(void) { return m_pCamera; }
	ITexture *getTexture(const char *szFilename) { return _TextureExists(szFilename); }

	void isBackgroundStatic(bool isStatic) { m_bStaticBG = isStatic; }
	void SetWidth(int nWidth) { m_nWidth = nWidth; }
	void SetHeight(int nHeight) { m_nHeight = nHeight; }
	void SetClearColor(color clearColor)
	{
		m_ClearColor = clearColor;
		m_bStaticBG = true;
	}
	void SetCamera(Camera *pCamera);

	virtual ITexture *CreateTexture(const char *szFilename, color colorKey = 0) = 0;
	virtual bool DestroyTexture(const ITexture *pTexture);

	void PushRenderList(RenderList *pRenderList) { _RenderLists.Store(pRenderList); }
	void PopRenderList(void) { _RenderLists.Erase(_RenderLists.End()); }

	RenderList *CreateRenderList(void) { return _RenderLists.Create(); }
	void DestroyRenderList(RenderList *list) { _RenderLists.Destroy(list); }

	virtual void Initialize(void) = 0;
	virtual void Shutdown(void) = 0;
	virtual void Render(void) = 0;
};

#endif
