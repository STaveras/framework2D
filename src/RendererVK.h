// RendererVK.h
#pragma once

#include "Renderer.h"

#include <vector>

#if __APPLE__
#include <MoltenVK/mvk_vulkan.h>
#include <MoltenVK/vk_mvk_moltenvk.h>
#else
#include <vulkan/vulkan.h>
#endif

class RendererVK : public IRenderer
{
    VkInstance _instance = VK_NULL_HANDLE;
    
    VkDevice _device = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain;
    VkExtent2D swapChainExtent;
    VkFormat swapChainImageFormat;
    std::vector<VkImage> swapChainImages;

    #if !defined(NDEBUG)
    VkDebugUtilsMessengerEXT debugMessenger;
    #endif

    void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window);

public:
    RendererVK(void);

    void Initialize(void);
    void Shutdown(void);
    void Render(void);

    ITexture *CreateTexture(const char *szFilename, color colorKey = 0);
};