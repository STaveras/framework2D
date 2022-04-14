// File: Window.h
#pragma once

#include "Types.h"

class Window
{
	bool m_bHasQuit;
	int m_nWidth;
	int m_nHeight;
	const char* m_szWindowTitle;
	const char* m_szWindowClassName;
#ifndef _WIN32
	GLFWwindow* _window;
public:
	GLFWwindow * getUnderlyingWindow(void) { return _window; }
private:
#else
	struct {
		HWND m_hWnd;
		HDC m_hDC;
		HINSTANCE m_hInstance;
		LPSTR m_lpCmdLine;
	}_window{};

	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

	void _resize(void);

protected:
	friend class RendererVK;

public:
	Window(void);
	Window(int nWidth, int nHeight, const char* szWindowTitle, const char* szWindowClassName = "");
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