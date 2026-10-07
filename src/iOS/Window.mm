// Window.mm
// Window on iOS: a thin wrapper around the game's UIView. UIKit owns the run
// loop and sizes the view, so update(), resize() and the fullscreen toggle do
// nothing. Sizes are in points; the client area is the part of the screen
// clear of the sensor housing (the safe area, left and right).
#if !__has_feature(objc_arc)
#error "Window.mm must be compiled with -fobjc-arc"
#endif

#include "../Window.h"

#import <UIKit/UIKit.h>

#include <cmath>

namespace {
UIView* nativeView(const Window& window)
{
	return (__bridge UIView*)window.getNativeView();
}
}

void Window::initialize(ClientAPI clientAPI, bool requireVulkanSupport)
{
	if (nativeView(*this)) {
		m_nWidth = getClientWidth();
		m_nHeight = getClientHeight();
	}
}

void Window::update(void)
{
	// Events reach the app through UIKit; there is nothing to poll.
}

void Window::shutdown(void)
{
}

void Window::setWindowTitle(const char* szWindowTitle)
{
	if (szWindowTitle) {
		m_szWindowTitle = szWindowTitle;
	}
}

int Window::getClientWidth(void) const
{
	UIView* view = nativeView(*this);
	if (!view) {
		return m_nWidth;
	}
	const UIEdgeInsets safe = view.safeAreaInsets;
	return (int)std::lround(view.bounds.size.width - safe.left - safe.right);
}

int Window::getClientHeight(void) const
{
	UIView* view = nativeView(*this);
	return view ? (int)std::lround(view.bounds.size.height) : m_nHeight;
}

void Window::resize(void)
{
}

void Window::toggleFullscreen(void)
{
}

bool Window::saveScreenshot(const std::string& path)
{
	// There is no OpenGL back buffer to read; RendererMTL captures frames itself.
	return false;
}
