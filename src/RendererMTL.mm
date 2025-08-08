// File: RendererMTL.cpp
#ifdef __APPLE__

#include "RendererMTL.h"
#include "TextureMTL.h"

// Default constructor
RendererMTL::RendererMTL(void)
{
    // Default values can be set here if needed
    initialize();
}

// Parameterized constructor
RendererMTL::RendererMTL(NSWindow* window, int nWidth, int nHeight, bool bFullscreen, bool bVsync)
{
    m_window = window;

    // Additional setup based on provided parameters
    initialize();
}

// Destructor
RendererMTL::~RendererMTL(void)
{
    shutdown();
}

// Initialization method
void RendererMTL::initialize(void)
{
    m_device = MTLCreateSystemDefaultDevice();

    m_view.device = m_device;

    if (m_device) {

      m_commandQueue = [m_device newCommandQueue];

      // Set up the Metal layer
      m_metalLayer = [CAMetalLayer layer];
      m_metalLayer.device = m_device;
      m_metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;

      m_window.contentView = m_view;
      // Additional setup for the Metal layer based on your requirements
      m_window.viewsNeedDisplay = true;
    }
    else {
      NSLog(@"Metal is not supported on this device");
      return;
    }

    // Additional initialization code here
}

// Create Texture method
ITexture* RendererMTL::createTexture(const char* szFilename, Color colorKey)
{
    // Create a new TextureMTL object
    TextureMTL* textureMTL = new TextureMTL(szFilename);

    // Load the texture data from the file
    // Set the Metal texture to textureMTL->_texture
    // Set the width and height to textureMTL->_width and textureMTL->_height
    // Additional logic to load the texture based on your requirements

    return textureMTL;
}

// Destroy Texture method
void RendererMTL::destroyTexture(ITexture* texture)
{
    // Release the texture resources if needed
    delete texture;
}

// Shutdown method
void RendererMTL::shutdown(void)
{
    // m_commandQueue.release();
    // Release Metal objects and any other resources
    // Additional cleanup logic based on your requirements
}

// Render method
void RendererMTL::render(void)
{
    
    // // Obtain a drawable object
    // m_drawable = [m_metalLayer nextDrawable];
    
    // if (!m_drawable) {
    //     return; // Return if no drawable available
    // }

    // // Create a command buffer and render pass descriptor
    // id<MTLCommandBuffer> commandBuffer = [m_commandQueue commandBuffer];

    // MTLRenderPassDescriptor* passDescriptor = m_view.currentRenderPassDescriptor;
    // passDescriptor.colorAttachments[0].clearColor = m_ClearColor;

    // // Create a render command encoder
    // id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];

    // // Draw your scene here (e.g., call _drawImage for each sprite)

    // // Finalize rendering
    // [renderEncoder endEncoding];
    // [commandBuffer presentDrawable:m_drawable];
    // [commandBuffer commit];

    // [m_view draw];
}

// Additional private methods (e.g., _drawImage) can be implemented here
#endif