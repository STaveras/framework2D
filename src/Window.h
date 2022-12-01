// File: Window.h
#pragma once

#include "Types.h"

class Window
{
	bool m_bHasQuit;
	int m_nWidth;
	int m_nHeight;
#ifdef _WIN32
	HWND		 m_hWnd = NULL;
	HDC		 m_hDC = NULL;
	HINSTANCE m_hInstance = NULL;
	LPSTR		 m_lpCmdLine = NULL;
#endif
	GLFWwindow* _window;

	const char* m_szWindowTitle;
	const char* m_szWindowClassName;
#ifdef _WIN32
	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
protected:
	friend class RendererVK;

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
	const char* getWindowTitle(void) const { return m_szWindowTitle; }
	const char* getWindowClassName(void) const { return m_szWindowClassName; }

	// Re-implement these so that setting them here actually resizes the window
	void setWidth(int nWidth);
	void setHeight(int nHeight);
	void setWindowTitle(const char* szWindowTitle);
#ifdef _WIN32
	void initialize(HINSTANCE hInstance, LPSTR lpCmdLine);
#endif
	void initialize(void);
	void update(void);
	void shutdown(void);

	void resize(void);
};
// Author: Stanley Taveras