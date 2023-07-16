// RendererVK.h
#pragma once

#include "Renderer.h"

#include "EventSystem.h"
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

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    bool isDeviceSuitable(VkPhysicalDevice device);
    void pickPhysicalDevice(VkInstance instance);
    void createLogicalDevice(VkPhysicalDevice physicalDevice);
    void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window); // TODO: Swap to "Window"
    void recreateSwapChain(void);
    void cleanupSwapChain(void);

    void createImageViews(VkDevice device);

    void createRenderPass(VkDevice device);

    void createGraphicsPipeline(VkDevice device);

    void createFramebuffers(VkDevice device);    
    
    void createCommandPool(VkDevice device);
    void createCommandBuffers(VkDevice device);

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createSyncObjects(void); // I don't necessarily want to even declare this here

    void OnWindowResized(const Event& e);

public:
    void initialize(void);
    void shutdown(void);
    void render(void);

    VkDevice getDevice(void) const { return _device; }

    ITexture *createTexture(const char *szFilename, Color colorKey = 0);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
};