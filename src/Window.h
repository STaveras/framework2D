// File: Window.h
#pragma once

#include "Types.h"
#include "Maths.h"

#include <string>

#include <functional>

#define EVT_WINDOW_RESIZED "EVT_WINDOW_RESIZED"

class Window
{
public:
	typedef std::function<void(int key, int scancode, int action, int mods)> KeyEventHandler;

private:
	bool m_bHasQuit;
	int m_nWidth;
	int m_nHeight;
#ifdef _WIN32
	HWND		 m_hWnd = NULL;
	HDC		 m_hDC = NULL;
	HINSTANCE m_hInstance = NULL;
	LPSTR		 m_lpCmdLine = NULL;
	WINDOWPLACEMENT m_wpPrev;
#endif
	GLFWwindow* _window;

	const char* m_szWindowClassName;
	std::string m_szWindowTitle;

	// Frame capture for verification runs (AUTO_SCREENSHOT_FRAME / AUTO_SCREENSHOT_PATH).
	long _renderedFrames = 0;
	long _autoScreenshotFrame = -1;
	std::string _autoScreenshotPath;

	KeyEventHandler m_keyEventHandler;

public:
	enum class ClientAPI
	{
		None,
		OpenGL
	};

	//constexpr static const char* EVT_WINDOW_RESIZED = "EVT_WINDOW_RESIZED";

	//class EventListener : public Cyclable
	//{
	//public:
	//	virtual void onWindowResized(const Event& evt) = 0;

	//	virtual ~EventListener() {}

	//	virtual void start(void) {
	//		//Engine2D::getInstance()->getEventSystem()->registerCallback<EventListener>(EVT_WINDOW_RESIZED, this, &EventListener::onWindowResized);
	//	}

	//	virtual void update(float time) {
	//		throw std::runtime_error("Window::EventListener::update() not implemented.");
	//	}

	//	virtual void finish(void) {
	//		//Engine2D::getInstance()->getEventSystem()->unregister<EventListener>(EVT_WINDOW_RESIZED, this, &EventListener::onWindowResized);
	//	}
	//};

private:
#ifdef _WIN32
	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
protected:
	friend class RendererVK; // HACK: This shouldn't be necessary, but we need to access the underlying window handle for Vulkan

public:
	Window(void);
	Window(int nWidth, int nHeight, const char* szWindowTitle, const char* szWindowClassName = "");
	~Window(void){}

#ifdef _WIN32
	HWND getHWND(void) const { return m_hWnd; }
	HDC getHDC(void) const { return m_hDC; }
	HINSTANCE getHINSTANCE(void) const { return m_hInstance; }
	LPSTR getCmdLineArgs(void) const { return m_lpCmdLine; }
#endif
	GLFWwindow * getUnderlyingWindow(void) { return _window; }

	bool hasQuit(void) const { return m_bHasQuit; }
	int getWidth(void) const { return m_nWidth; }
	int getHeight(void) const { return m_nHeight; }
	int getClientWidth(void) const;
	int getClientHeight(void) const;

	const char* getWindowTitle(void) const { return m_szWindowTitle.c_str(); }
	const char* getWindowClassName(void) const { return m_szWindowClassName; }

	// Re-implement these so that setting them here actually resizes the window
	void setWidth(int nWidth);
	void setHeight(int nHeight);
	void setWindowTitle(const char* szWindowTitle);

	// Receives every GLFW key event as it is delivered by glfwPollEvents()
	void setKeyEventHandler(KeyEventHandler handler) { m_keyEventHandler = std::move(handler); }
#ifdef _WIN32
	void initialize(HINSTANCE hInstance, LPSTR lpCmdLine);
#endif
	void initialize(ClientAPI clientAPI = ClientAPI::None, bool requireVulkanSupport = false);
	void update(void);
	void shutdown(void);

	void resize(void);
	void toggleFullscreen(void);

	// Saves the current OpenGL back buffer as a PNG. Call after a frame is drawn and before
	// it is presented. Returns false without an OpenGL context or if writing fails.
	bool saveScreenshot(const std::string& path);

	// Called by the renderer once a frame is drawn, before presenting it. viewMin/viewMax is
	// the world rectangle the frame shows. With AUTO_SCREENSHOT_FRAME=N and
	// AUTO_SCREENSHOT_PATH set, frame N is saved there, with the view in "<path>.view".
	void onFrameRendered(const vector2& viewMin, const vector2& viewMax);
};
// Author: Stanley Taveras
