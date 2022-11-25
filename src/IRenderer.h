// File: IRenderer.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/28/2010

#if !defined(_IRENDERER_H_)
#define _IRENDERER_H_

#include "Factory.h"
#include "ITexture.h"
#include <list>

class Camera;

typedef class IRenderer
{
public:
	typedef struct RenderList : public std::list<class Renderable *>
	{
		// TODO: Add other state information
	} RenderList;

protected:
#if _DEBUG
	void _BackgroundColorShift(void);

	bool m_bStaticBG; // For demoing
#endif
	bool m_bFullScreen;
	bool m_bVerticalSync;
	int m_nWidth;
	int m_nHeight;
	Color m_ClearColor;
	Camera *m_pCamera; // This is going to change into a list of cameras
	Factory<ITexture> m_Textures;
	Factory<RenderList> _RenderLists;

	ITexture *_TextureExists(const char *szFilename);

public:
	IRenderer(void) :
#if _DEBUG
		m_bStaticBG(false),
#endif
		m_bFullScreen(false),
		m_bVerticalSync(false),
		m_nWidth(0), m_nHeight(0),
		m_ClearColor(0xFFFFFFFF),
		m_pCamera(NULL)
	{
		_RenderLists.create();
	} // Comes with one global render list

	IRenderer(int nWidth, int nHeight, bool fullscreen = false, bool vsync = false) :
#if _DEBUG
		m_bStaticBG(false),
#endif
		m_bFullScreen(fullscreen),
		m_bVerticalSync(vsync),
		m_nWidth(nWidth),
		m_nHeight(nHeight),
		m_ClearColor(0xFFFFFFFF),
		m_pCamera(NULL) {}

	virtual ~IRenderer() = 0;

#if _DEBUG
	bool isBackgroundStatic(void) const { return m_bStaticBG; }
#endif
	bool isFullScreen(void) const { return m_bFullScreen; }
	bool verticalSyncEnabled(void) const { return m_bVerticalSync; };
	int getWidth(void) const { return m_nWidth; }
	int getHeight(void) const { return m_nHeight; }
	Color GetClearColor(void) const { return m_ClearColor; }
	Camera *GetCamera(void) { return m_pCamera; }
	ITexture *getTexture(const char *szFilename) { return _TextureExists(szFilename); }


#if _DEBUG
	void isBackgroundStatic(bool isStatic) { m_bStaticBG = isStatic; }
#endif
	void setWidth(int nWidth) { m_nWidth = nWidth; }
	void setHeight(int nHeight) { m_nHeight = nHeight; }
	void SetClearColor(Color clearColor);
	void SetCamera(Camera *pCamera);

	virtual void setFullScreen(bool isFullScreen) { m_bFullScreen = isFullScreen; }
	virtual void setVerticalSync(bool vsyncEnabled) { m_bVerticalSync = vsyncEnabled; }

	virtual ITexture *createTexture(const char *szFilename, Color colorKey = 0) = 0;
	virtual bool destroyTexture(const ITexture *pTexture);

	void pushRenderList(RenderList *pRenderList) { _RenderLists.store(pRenderList); }
	void popRenderList(void) { _RenderLists.erase(_RenderLists.end()); }

	RenderList *CreateRenderList(void) { return _RenderLists.create(); }
	void DestroyRenderList(RenderList *list) { _RenderLists.destroy(list); }

	virtual void initialize(void) = 0;
	virtual void shutdown(void) = 0;
	virtual void render(void) = 0;
	
} RenderingInterface;

#endif
