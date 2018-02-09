
#include "Renderer.h"
#include "Engine2D.h"

IRenderer* Renderer::Get(void) { return Engine2D::getInstance()->GetRenderer(); }

// This function will be replaced with generic functions and will allow you to select a renderer module, to allow better encapsulation and extensibility
IRenderer* Renderer::CreateDXRenderer(HWND hWnd, int nWidth, int nHeight, bool bFullscreen, bool bVsync)
{
	IRenderer* renderer = new RendererDX(hWnd, nWidth, nHeight, bFullscreen, bVsync);
	return renderer;
}

void Renderer::DestroyRenderer(IRenderer* pRenderer)
{ 
	if(pRenderer)
	{ 
		pRenderer->Shutdown(); 
		delete pRenderer; 
	} 
}

//void Renderer::Render(Renderable* r)
//{
//    // TODO: Improve this massively.
//    IRenderer::RenderList* renderList = Engine2D::getInstance()->GetRenderer()->CreateRenderList();
//    renderList->push_back(r);
//    Engine2D::getInstance()->GetRenderer()->DestroyRenderList(renderList);
//}
//
//void Renderer::AddToRenderList(IRenderer::renderList* pRenderList, Renderable* pRenderable) { pRenderList->push_back(pRenderable); }
//bool Renderer::RemoveFromRenderList(IRenderer::renderList* pRenderList, Renderable* pRenderable)
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
//}
//
//void Renderer::AddToRenderList(Renderable* pRenderable) { AddToRenderList(&g_RenderList, pRenderable); }
//bool Renderer::RemoveFromRenderList(Renderable* pRenderable) { return RemoveFromRenderList(&g_RenderList, pRenderable); }