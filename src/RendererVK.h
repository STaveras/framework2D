// RendererVK.h
#pragma once

#include "Renderer.h"

#include "ImageLoaders.h"

#include <optional>
#include <vector>

class RendererVK : public IRenderer
{
    VkInstance _instance = VK_NULL_HANDLE;
    
    VkDevice _device = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;

    // Window presentation surface
    VkSurfaceKHR _surface = VK_NULL_HANDLE;

    VkQueue _presentQueue = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;

    VkPipeline _graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    VkRenderPass _renderPass = VK_NULL_HANDLE;

    VkCommandPool _commandPool;

    uint32_t currentFrame = 0;

    std::vector<VkFence> inFlightFences;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;

    std::vector<VkCommandBuffer> _commandBuffers;

    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    VkExtent2D _swapChainExtent;
    VkFormat _swapChainImageFormat;

    std::vector<VkImage> _swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    std::vector<VkShaderModule> _shaderModules;

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

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char> &code);

    bool isDeviceSuitable(VkPhysicalDevice device);
    void pickPhysicalDevice(VkInstance instance);
    void createLogicalDevice(VkPhysicalDevice physicalDevice);
    void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window);

    void createImageViews(VkDevice device);

    void createRenderPass(VkDevice device);

    void createGraphicsPipeline(VkDevice device);

    void createFramebuffers(VkDevice device);    
    
    void createCommandPool(VkDevice device);
    void createCommandBuffers(VkDevice device);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createSyncObjects(void); // I don't necessarily want to even declare this here

public:

    void Initialize(void);
    void Shutdown(void);
    void Render(void);

    ITexture *CreateTexture(const char *szFilename, color colorKey = 0);
};