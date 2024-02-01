// File: RendererMTL.h
#ifdef __APPLE__
#ifndef _RENDERERMTL_H
#define _RENDERERMTL_H

#include "IRenderer.h"
#include <Cocoa/Cocoa.h>
#include <Metal/Metal.h>
#include <QuartzCore/CAMetalLayer.h>

class Sprite;
class TextureMTL;

class RendererMTL : public IRenderer
{
   NSWindow* m_window;
   id<MTLDevice> m_device;
   id<MTLCommandQueue> m_commandQueue;
   CAMetalLayer* m_metalLayer;
   id<CAMetalDrawable> m_drawable;
   id<MTLRenderPipelineState> m_pipelineState;

   void _drawImage(Sprite* pSprite, Color tint = 0xFFFFFFFF, vector2 offset = {0.0f, 0.0f}, float zValue = 0.0f);

public:
   RendererMTL(void);
   RendererMTL(NSWindow* window, int nWidth, int nHeight, bool bFullscreen = false, bool bVsync = false);
   ~RendererMTL(void);

   NSWindow* getWindow(void) const { return m_window; }
   id<MTLDevice> getDevice(void) const { return m_device; }

private:
   ITexture* createTexture(const char* szFilename, Color colorKey = 0);
   void destroyTexture(ITexture* texture);

   void initialize(void);
   void shutdown(void);
   void render(void);
};

#endif //_RENDERERMTL_H
#endif //__APPLE__
// Author: YOUR_NAME_HERE
