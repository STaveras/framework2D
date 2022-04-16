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

    // Window presentation surface
    VkSurfaceKHR _surface = VK_NULL_HANDLE;

    VkQueue _presentQueue = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;

    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    VkExtent2D _swapChainExtent;
    VkFormat _swapChainImageFormat;
    std::vector<VkImage> swapChainImages;

    #if !defined(NDEBUG)
    VkDebugUtilsMessengerEXT debugMessenger;
    #endif

    struct QueueFamilyIndices {

        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    QueueFamilyIndices findQueueFamilies(void);
    SwapChainSupportDetails querySwapChainSupport(void);

    bool isDeviceSuitable(VkPhysicalDevice device);
    void pickPhysicalDevice(VkInstance instance, VkPhysicalDevice& physicalDevice);
    void createLogicalDevice(VkPhysicalDevice physicalDevice, VkDevice& device);
    void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window);

public:
    RendererVK(void);

    void Initialize(void);
    void Shutdown(void);
    void Render(void);

    ITexture *CreateTexture(const char *szFilename, color colorKey = 0);
};