
#include "Renderer.h"
#include "Engine2D.h"

Window *Renderer::mainWindow = NULL;

IRenderer *Renderer::get(void) { return Engine2D::getRenderer(); }

#if _WIN32
IRenderer *Renderer::createDXRenderer(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync) {
	return ((IRenderer *)new RendererDX(hWnd, nWidth, nHeight, bFullscreen, bVsync));
}
#elif __APPLE__
IRenderer *Renderer::createMTLRenderer(Window *window)
{
	IRenderer* renderer = new RendererMTL(glfwGetCocoaWindow(window->getUnderlyingWindow()), 
										  window->getWidth(), window->getHeight(), 
										  false);
	return renderer;
}
#endif

// We should probably do as above, and allow dimensions and other settings to be specified from the get go, instead of just being inferred from the window properties...?
IRenderer* Renderer::createVKRenderer(Window* window)
{
	IRenderer* renderer = new RendererVK();
	renderer->setWidth(window->getWidth());
	renderer->setHeight(window->getHeight());
	return renderer;
}

IRenderer* Renderer::createGLRenderer(Window* window)
{
	IRenderer* renderer = new RendererGL(window);
	if (window) {
		renderer->setWidth(window->getWidth());
		renderer->setHeight(window->getHeight());
	}
	return renderer;
}

void Renderer::destroyRenderer(IRenderer *pRenderer)
{
	if (pRenderer)
	{
		pRenderer->shutdown();
		delete pRenderer;
	}
}
