
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
	return new RendererMTL(window);
}
#endif

#if !FRAMEWORK_IOS
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
#endif

void Renderer::destroyRenderer(IRenderer *pRenderer)
{
	if (pRenderer)
	{
		pRenderer->shutdown();
		delete pRenderer;
	}
}

// void Renderer::render(Renderable* r)
//{
//     // TODO: Improve this massively.
//     IRenderer::RenderList* renderList = Engine2D::getInstance()->GetRenderer()->createRenderList();
//     renderList->push_back(r);
//     Engine2D::getInstance()->GetRenderer()->destroyRenderList(renderList);
// }
//
// void Renderer::AddToRenderList(IRenderer::renderList* pRenderList, Renderable* pRenderable) { pRenderList->push_back(pRenderable); }
// bool Renderer::RemoveFromRenderList(IRenderer::renderList* pRenderList, Renderable* pRenderable)
//{
//	std::list<Renderable*>::iterator itr = pRenderList->begin();
//	for(;itr != pRenderList->end();itr++)
//	{
//		if((*itr) == pRenderable)
//		{
//			pRenderList->erase(itr);
//			return true;
//		}
//	}
//
//	return false;
// }
//
// void Renderer::AddToRenderList(Renderable* pRenderable) { AddToRenderList(&g_RenderList, pRenderable); }
// bool Renderer::RemoveFromRenderList(Renderable* pRenderable) { return RemoveFromRenderList(&g_RenderList, pRenderable); }
