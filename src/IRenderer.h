// File: IRenderer.h
// Author: Stanley Taveras
// Created: 2/24/2010
// Modified: 2/28/2010

#if !defined(_IRENDERER_H_)
#define _IRENDERER_H_

#include "Factory.h"
#include "ITexture.h"
#include <list>
#include <unordered_map>
#include <vector>

class Camera;

typedef class IRenderer
{
public:
typedef struct RenderList : public std::list<class Renderable *>
{
    bool screenSpace;

    RenderList()
        : screenSpace(false)
    {}
} RenderList;

	// Renderer API types
	// This is used to identify the type of renderer being used
	// NOTE: This might go away in the future, as we might want to use a more generic interface
	typedef enum RENDERER_API_TYPE
	{
		RENDERER_TYPE_NULL = 0,
		RENDERER_TYPE_DX,		// DirectX 9 Sprite renderer
		RENDERER_TYPE_MTL,	// Metal renderer (for macOS/iOS)
		RENDERER_TYPE_VK,		// Vulkan renderer
		RENDERER_TYPE_GL		// OpenGL renderer
	}TYPE;

protected:
	void _backgroundColorShift(void);

	bool m_bStaticBG; // For demoing
	std::vector<Color> m_BackgroundColorPoints;
	size_t m_BackgroundColorStage;
	float m_BackgroundColorProgress;
	bool m_bFullScreen;
	bool m_bVerticalSync;
	int m_nWidth;
	int m_nHeight;
	Color m_ClearColor;
	Camera *m_pCamera; // This is going to change into a list of cameras
	Factory<ITexture> m_Textures;
	Factory<RenderList> _RenderLists;

	RENDERER_API_TYPE _type;

	ITexture *_textureExists(const char *szFilename);

public:
	IRenderer(RENDERER_API_TYPE renderingAPI = RENDERER_TYPE_NULL, 
				 int nWidth = 0, int nHeight = 0,
				 bool fullscreen = false, 
				 bool vsync = false) :
		m_bStaticBG(false),
		m_BackgroundColorPoints({
			Color(0xFFFF0000), // red
			Color(0xFFFFA500), // orange
			Color(0xFFFFFF00), // yellow
			Color(0xFF00FF00), // green
			Color(0xFF0000FF), // blue
			Color(0xFF800080)  // purple
			}),
		m_BackgroundColorStage(0),
		m_BackgroundColorProgress(0.0f),
		m_bFullScreen(fullscreen),
		m_bVerticalSync(vsync),
		m_nWidth(nWidth), m_nHeight(nHeight),
		m_ClearColor(0xFFFFFFFF),
		m_pCamera(NULL),
		_type(renderingAPI){
		// Comes with one global render list
		_RenderLists.create();
	} 

	virtual ~IRenderer() = 0;

	bool isBackgroundStatic(void) const { return m_bStaticBG; }
	void setBackgroundStatic(bool isStatic) { m_bStaticBG = isStatic; }
	const std::vector<Color>& getBackgroundColorPoints(void) const { return m_BackgroundColorPoints; }
	void setBackgroundColorPoints(const std::vector<Color>& points);
	void resetBackgroundColorShift(void);

	bool isFullScreen(void) const { return m_bFullScreen; }
	bool verticalSyncEnabled(void) const { return m_bVerticalSync; };
	int getWidth(void) const { return m_nWidth; }
	int getHeight(void) const { return m_nHeight; }
	Color getClearColor(void) const { return m_ClearColor; }
	Camera *getCamera(void) { return m_pCamera; }
	ITexture *getTexture(const char *szFilename) { return _textureExists(szFilename); }

	void setWidth(int nWidth) { m_nWidth = nWidth; }
	void setHeight(int nHeight) { m_nHeight = nHeight; }
	void setClearColor(Color clearColor);
	void setCamera(Camera *pCamera);

	virtual void setFullScreen(bool isFullScreen) { m_bFullScreen = isFullScreen; }
	virtual void setVerticalSync(bool vsyncEnabled) { m_bVerticalSync = vsyncEnabled; }

	virtual ITexture *createTexture(const char *szFilename, Color colorKey = 0) = 0;
	virtual bool destroyTexture(const ITexture *pTexture);

	void pushRenderList(RenderList *pRenderList) { _RenderLists.store(pRenderList); }
	void popRenderList(void) { _RenderLists.erase(_RenderLists.end()); }

    RenderList *createRenderList(bool screenSpace = false)
    {
        RenderList *renderList = _RenderLists.create();
        if (renderList)
            renderList->screenSpace = screenSpace;
        return renderList;
    }
	void destroyRenderList(RenderList *list) { _RenderLists.destroy(list); }

	virtual void initialize(void) = 0;
	virtual void shutdown(void) = 0;
	virtual void render(void) = 0;

	TYPE renderingAPI(void) const { return _type; }
	
} RenderingInterface;

// Mapping from enum to string
static const std::unordered_map<RenderingInterface::TYPE, std::string> apiTypeToString{
	 { RenderingInterface::TYPE::RENDERER_TYPE_DX, "DirectX9" },
	 { RenderingInterface::TYPE::RENDERER_TYPE_GL, "OpenGL" },
	 { RenderingInterface::TYPE::RENDERER_TYPE_VK, "Vulkan" }
};

// Mapping from string to enum
static const std::unordered_map<std::string, RenderingInterface::TYPE> stringToAPIType{
	 { "DirectX9",  RenderingInterface::TYPE::RENDERER_TYPE_DX},
	 { "OpenGL", RenderingInterface::TYPE::RENDERER_TYPE_GL },
	 { "Vulkan",  RenderingInterface::TYPE::RENDERER_TYPE_VK }
};

namespace RENDERER_API_TYPE {

	// Convert enum to string
	static std::string toString(RenderingInterface::TYPE type) {
		auto it = apiTypeToString.find(type);
		if (it != apiTypeToString.end()) {
			return it->second;
		}
		throw std::invalid_argument("Invalid API enum value");
	}
}

#endif
