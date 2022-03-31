// File: Window.h
#pragma once
#ifdef _WIN32
#include <windows.h>
#else
#define GLFW_BUILD_UNIVERSAL
#define GLFW_USE_MENUBAR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
class Window
{
	bool m_bHasQuit;
	int m_nWidth;
	int m_nHeight;
#ifdef _WIN32
	HWND m_hWnd;
	HDC m_hDC;
	HINSTANCE m_hInstance;
	LPSTR m_lpCmdLine;
#else
	GLFWwindow* _window;
#endif
	const char* m_szWindowTitle;
	const char* m_szWindowClassName;

	void _resize(void);

#ifdef _WIN32
	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

public:
	Window(void);
	Window(int nWidth, int nHeight, const char* szWindowTitle, const char* szWindowClassName = "2DEF");
	~Window(void){}

	bool HasQuit(void) const { return m_bHasQuit; }
	int GetWidth(void) const { return m_nWidth; }
	int GetHeight(void) const { return m_nHeight; }
#ifdef _WIN32
	HWND GetHWND(void) const { return m_hWnd; }
	HDC GetHDC(void) const { return m_hDC; }
	HINSTANCE GetHINSTANCE(void) const { return m_hInstance; }
	LPSTR GetCmdLineArgs(void) const { return m_lpCmdLine; }
#endif
	const char* GetWindowTitle(void) const { return m_szWindowTitle; }
	const char* GetWindowClassName(void) const { return m_szWindowClassName; }

	void SetWidth(int nWidth) { m_nWidth = nWidth; }
	void SetHeight(int nHeight) { m_nHeight = nHeight; }
	void SetWindowTitle(const char* szWindowTitle) { m_szWindowTitle = szWindowTitle; }

#ifdef _WIN32
	void Initialize(HINSTANCE hInstance, LPSTR lpCmdLine);
#else
	void Initialize(void);
#endif
	void Update(void);
	void Shutdown(void);
};
// Author: Stanley Taveras