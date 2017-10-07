// File: Window.h
#pragma once
#include <windows.h>
class Window
{
	bool m_bHasQuit;
	int m_nWidth;
	int m_nHeight;
	HWND m_hWnd;
	HDC m_hDC;
	HINSTANCE m_hInstance;
	LPSTR m_lpCmdLine;
	const char* m_szWindowTitle;
	const char* m_szWindowClassName;

	void _resize(void);
	static LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	Window(void);
	Window(int nWidth, int nHeight, const char* szWindowTitle, const char* szWindowClassName);
	~Window(void){}

	bool HasQuit(void) const { return m_bHasQuit; }
	int GetWidth(void) const { return m_nWidth; }
	int GetHeight(void) const { return m_nHeight; }
	HWND GetHWND(void) const { return m_hWnd; }
	HDC GetHDC(void) const { return m_hDC; }
	HINSTANCE GetHINSTANCE(void) const { return m_hInstance; }
	LPSTR GetCmdLineArgs(void) const { return m_lpCmdLine; }
	const char* GetWindowTitle(void) const { return m_szWindowTitle; }
	const char* GetWindowClassName(void) const { return m_szWindowClassName; }

	void SetWidth(int nWidth) { m_nWidth = nWidth; }
	void SetHeight(int nHeight) { m_nHeight = nHeight; }
	void SetWindowTitle(const char* szWindowTitle) { m_szWindowTitle = szWindowTitle; }

	void Initialize(HINSTANCE hInstance, LPSTR lpCmdLine);
	void Update(void);
	void Shutdown(void);
};
// Author: Stanley Taveras