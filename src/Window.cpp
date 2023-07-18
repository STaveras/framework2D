// File: Window.cpp
// Author: Stanley Taveras
// Created: 2/21/2010
// Modified: 8/23/2022
// TODO: Rethink how we handle resizing the window

#include "Window.h"
#include "Engine2D.h"

#include <iostream>

Window::Window(void) :
	m_bHasQuit(false),
	m_nWidth(GLOBAL_WIDTH),
	m_nHeight(GLOBAL_HEIGHT),
	m_szWindowTitle(""),
	m_szWindowClassName("_ENGINE_2D_WINDOW"),
	_window(NULL) {

}

Window::Window(int nWidth, int nHeight, const char* szWindowTitle, const char* szWindowClassName) :
	m_bHasQuit(false),
	m_nWidth(nWidth),
	m_nHeight(nHeight),
	m_szWindowTitle(szWindowTitle),
	m_szWindowClassName((!strcmp(szWindowClassName, "")) ? "_ENGINE_2D_WINDOW" : szWindowClassName),
	_window(NULL) {

}

#ifdef _WIN32
LRESULT WINAPI Window::MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_GETMINMAXINFO:
	case WM_SIZING:
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & (1 << 29))) { // Check if ALT key is pressed
			Renderer::window->toggleFullscreen();
			return 0;
		}
		return 0;
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Window::initialize(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	m_hInstance = hInstance;
	m_lpCmdLine = lpCmdLine;

	WNDCLASSEX wcex =
	{
		sizeof(WNDCLASSEX), CS_OWNDC | CS_DBLCLKS, this->MsgProc, 0L, 0L,
		hInstance, NULL, NULL, NULL, NULL,
		m_szWindowClassName, NULL
	};

	RegisterClassEx(&wcex);

	m_hWnd = CreateWindowEx(WS_EX_APPWINDOW,
									m_szWindowClassName, 
									m_szWindowTitle,
									WS_SYSMENU,/*For borderless FS: WS_POPUPWINDOW,*/
									CW_USEDEFAULT, 
									CW_USEDEFAULT, 
									m_nWidth,
									m_nHeight,
									NULL, NULL, hInstance, NULL);
	resize();

	ShowWindow(m_hWnd, SW_SHOWDEFAULT);
}
#endif

void Window::initialize(void) {

#if __APPLE__
	glfwInitHint(GLFW_COCOA_MENUBAR, GLFW_TRUE);
#else

#endif
	/* Initialize the library */
	if (!glfwInit())
		return; // -1

	if (glfwVulkanSupported() == GLFW_FALSE) {
		std::cout << "Vulkan is not supported on this platform." << std::endl;
		return;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	/* Create a windowed mode window and its OpenGL context */
	_window = glfwCreateWindow(m_nWidth, m_nHeight, m_szWindowTitle, NULL, NULL);
	if (!_window) {
		return glfwTerminate(); // -1 // Maybe throw an exception
	}

	glfwSetWindowUserPointer(_window, this);
	
	glfwSetFramebufferSizeCallback(_window, [](GLFWwindow* window, int width, int height) {

		Window* _window = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (_window->getUnderlyingWindow() == window) {

			_window->setWidth(width);
			_window->setHeight(height);
			
			Engine2D::getEventSystem()->sendEvent(EVT_WINDOW_RESIZED, window);
		}
	});

	// Register a key callback function
	glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {

		Window* _window = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && mods == GLFW_MOD_ALT)
		{
			// Check if the window is currently fullscreen
			bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;

			// Get the primary monitor
			GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

			if (isFullscreen) {
				// Switch back to windowed mode
				glfwSetWindowMonitor(window, nullptr, 0, 0, _window->getWidth(), _window->getHeight(), GLFW_DONT_CARE);
			}
			else {
				// Switch to fullscreen mode
				const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
				glfwSetWindowMonitor(window, primaryMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			}
		}
	});

	// Add fullscreen handling
	glfwSetWindowFocusCallback(_window, [](GLFWwindow* window, int focused) {

		Window* _window = static_cast<Window*>(glfwGetWindowUserPointer(window));

		if (focused) {
			// Window gained focus
			bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
			if (isFullscreen) {
				// Set the window to fullscreen mode
				glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, _window->getWidth(), _window->getHeight(), GLFW_DONT_CARE);
			}
			}
		else {
			// Window lost focus
			bool isFullscreen = glfwGetWindowMonitor(window) != nullptr;
			if (isFullscreen) {
				// Switch back to windowed mode
				glfwSetWindowMonitor(window, nullptr, 0, 0, _window->getWidth(), _window->getHeight(), GLFW_DONT_CARE);
			}
		}
	});

	/* Make the window's context current */
	glfwMakeContextCurrent(_window);
}

void Window::update(void)
{
	if (_window) {
		/* Loop until the user closes the window */
		m_bHasQuit = glfwWindowShouldClose(_window);

		/* Poll for and process events */
		glfwPollEvents();
	}
#ifdef _WIN32
	else {
		MSG msg;
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				m_bHasQuit = true;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		InvalidateRect(m_hWnd, NULL, true);
	}
#endif
}

void Window::shutdown(void)
{
	if (_window) {
		glfwDestroyWindow(_window);
		glfwTerminate();
	}
#ifdef _WIN32
	else {
		DestroyWindow(m_hWnd);
		UnregisterClass(m_szWindowClassName, m_hInstance);
	}
#endif
}

void Window::setWindowTitle(const char* szWindowTitle) 
{ 
	m_szWindowTitle = szWindowTitle; 

	if (_window) {
		glfwSetWindowTitle(_window, m_szWindowTitle);
	}
#ifdef _WIN32
	else {
		SetWindowTextA(m_hWnd, m_szWindowTitle);
	}
#endif
}

void Window::setWidth(int nWidth)
{
	if (m_nWidth != nWidth) {
		m_nWidth = nWidth;
	}
}

void Window::setHeight(int nHeight)
{
	if (m_nHeight != nHeight) {
		m_nHeight = nHeight;
	}
}

void Window::resize(void)
{
	if (_window) {

		int width = 0, height = 0; 
		glfwGetWindowSize(_window, &width, &height);

		if (m_nWidth != width || m_nHeight != height) {
			glfwSetWindowSize(_window, m_nWidth, m_nHeight);
		}
	}
#ifdef _WIN32
	else {
		RECT rcClient, rcWindow;
		POINT diff;

		// get the size of the current client area
		GetClientRect(m_hWnd, &rcClient);

		// get the size of the containing window
		GetWindowRect(m_hWnd, &rcWindow);

		// determine the difference between the two
		diff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
		diff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;

		// resize the client area
		MoveWindow(m_hWnd, rcWindow.left, rcWindow.top, (m_nWidth + diff.x), (m_nHeight + diff.y), TRUE);
	}
#endif
}

void Window::toggleFullscreen(void)
{
#ifdef _WIN32
	DWORD dwStyle = GetWindowLong(m_hWnd, GWL_STYLE);

	IRenderer* renderer = Renderer::get();

	if (dwStyle & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO mi = { sizeof(mi) };
		if (GetWindowPlacement(m_hWnd, &m_wpPrev) && GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY), &mi))
		{
			SetWindowLong(m_hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(m_hWnd, HWND_TOP,
				mi.rcMonitor.left, mi.rcMonitor.top,
				mi.rcMonitor.right - mi.rcMonitor.left,
				mi.rcMonitor.bottom - mi.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
		renderer->setFullScreen(true);
	}
	else
	{
		renderer->setFullScreen(false);

		SetWindowLong(m_hWnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(m_hWnd, &m_wpPrev);
		SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
#endif
}