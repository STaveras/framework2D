// Created:  3/31/2022
// Modified: 4/1/2022

#include "RendererVK.h"

// NOTE: We should try and decouple the ide of a "window" from this, considering we may want to use this to render off-screen

#include <iostream>
#include <set>

// Not sure how I feel about this, since it's a C++17 feature
#include <optional>

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp

// NOTE: We may need this in the future to directly handle memory
// typedef struct VkAllocationCallbacks {
//     void*                                   pUserData;
//     PFN_vkAllocationFunction                pfnAllocation;
//     PFN_vkReallocationFunction              pfnReallocation;
//     PFN_vkFreeFunction                      pfnFree;
//     PFN_vkInternalAllocationNotification    pfnInternalAllocation;
//     PFN_vkInternalFreeNotification          pfnInternalFree;
// }

VkAllocationCallbacks vkCallbacks{};

const std::vector<const char *> instanceExtensions = {
#if __APPLE__
    "VK_KHR_get_physical_device_properties2"
#endif
};

const std::vector<const char *> deviceExtensions = {
#if __APPLE__
    // MoltenVK is a portability subset
    "VK_KHR_portability_subset",
#endif
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;

VkDebugUtilsMessengerEXT debugMessenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::cerr << std::endl << "Vulkan -- " << pCallbackData->pMessage << std::endl;
    }
    return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) 
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr; // Optional
}

void setupDebugMessenger(VkInstance instance) 
{
    if (!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo);

    if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Vulkan: Failed to set up debug messenger!");
    }
}
#endif

RendererVK::RendererVK(void) : _instance(NULL), IRenderer()
{
    // Ehh
}

bool checkValidationLayerSupport(std::vector<const char *>  validationLayers) {

    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char *layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

int rateDeviceSuitability(VkPhysicalDevice device) {

    int score = 0;

    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;

    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // Discrete GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    // Metal (ergo, MoltenVK) does not support Geometry Shaders!!!
#if !(__APPLE__)
    if (!deviceFeatures.geometryShader) {
        return 0;
    }
#endif

    return score;
}

RendererVK::QueueFamilyIndices RendererVK::findQueueFamilies(VkPhysicalDevice device) {

    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto &queueFamily : queueFamilies) {

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

RendererVK::SwapChainSupportDetails RendererVK::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // Find the one we want...
    for (const auto &availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // all else fails?
    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool verticalSync = false) {

    // Check the vailable modes and filter for one of these two below... 

    return (verticalSync) ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR; // these two modes are definitely available
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow *window) {

    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)};

    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

bool RendererVK::isDeviceSuitable(VkPhysicalDevice device) {

    // // Use an ordered map to automatically sort candidates by increasing score
    // std::multimap<int, VkPhysicalDevice> candidates;

    // for (const auto& device : devices) {
    //     int score = rateDeviceSuitability(device);
    //     candidates.insert(std::make_pair(score, device));
    // }

    // // Check if the best candidate is suitable at all
    // if (candidates.rbegin()->first > 0) {
    //     physicalDevice = candidates.rbegin()->second;
    // } else {
    //     throw std::runtime_error("failed to find a suitable GPU!");
    // }

    // VkPhysicalDeviceProperties deviceProperties;
    // VkPhysicalDeviceFeatures deviceFeatures;

    // vkGetPhysicalDeviceProperties(device, &deviceProperties);
    // vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // // TODO: Sort out the necessary capabilities we need to render

    // return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainAdequate = false;

    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void RendererVK::pickPhysicalDevice(VkInstance instance) 
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto &device : devices) {
        if (device != VK_NULL_HANDLE) {
            if (isDeviceSuitable(device)) {
                _physicalDevice = device;
                break;
            }
        }
    }
}

void RendererVK::createLogicalDevice(VkPhysicalDevice physicalDevice) {

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    float queuePriority = 1.0f;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.queueCreateInfoCount = 1;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

void RendererVK::createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow *window) 
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // This would be 2 in VR!
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // y tho; i wanna find out if we can set this to mean that our world's coordinate system is oriented in the same way that it would be in DirectX
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // Screen-space-based algorithms may not work?
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;     // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    _swapChainImageFormat = surfaceFormat.format;
    _swapChainExtent = extent;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }
}

void RendererVK::Initialize(void)
{
    if (Renderer::window)
    {
        GLFWwindow *window = Renderer::window->getUnderlyingWindow();

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = Renderer::window->GetWindowTitle();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = Renderer::window->GetWindowClassName();
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        for (const char* extension: instanceExtensions) {
            extensions.push_back(extension);
        }

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

        if (enableValidationLayers) {

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            populateDebugMessengerCreateInfo(debugCreateInfo);

            if (!checkValidationLayerSupport(validationLayers)) {
                throw std::runtime_error("Validation layers requested, but not available!");
            }
            else {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
                createInfo.pNext = &debugCreateInfo;
            }
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        switch (vkCreateInstance(&createInfo, nullptr, &_instance))
        {
        case VK_SUCCESS:
            std::cout << "Vulkan initialized" << std::endl;
            break;
        default:
            throw std::runtime_error("Failed to create Vulkan instance!");
        }

        if (glfwCreateWindowSurface(_instance, window, nullptr, &_surface) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create window surface!");
        }

        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensionProperties(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensionProperties.data());

        std::cout << "Available extensions:\n";

        for (const auto &extension : extensionProperties) {
            std::cout << '\t' << extension.extensionName << '\n';
        }

        if (enableValidationLayers) {
            setupDebugMessenger(_instance);
        }

        pickPhysicalDevice(_instance);
        createLogicalDevice(_physicalDevice);
        createSwapChain(_physicalDevice, _device, window);

        // we might have to wait a lil bit for this to work? (grabbing a "swapChain" apparently takes a while)

        if (_swapChain != VK_NULL_HANDLE) {
            uint32_t imageCount = 0;
            vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
            swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, swapChainImages.data());
        }
    }
}

void RendererVK::Shutdown(void)
{
    if (_instance) {

        // for(auto &swapChainImage : swapChainImages) {
        //     vkDestroyImageView(_device, imageView, nullptr);
        // }

        if (_swapChain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(_device, _swapChain, nullptr);

        if (_surface != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(_instance, _surface, nullptr);

        if (_device != VK_NULL_HANDLE)
            vkDestroyDevice(_device, nullptr);

        if (enableValidationLayers) {
            destroyDebugUtilsMessengerEXT(_instance, debugMessenger, nullptr);
        }

        vkDestroyInstance(_instance, nullptr);
    }
}

void RendererVK::Render(void)
{
    IRenderer::Render();

    if (_device == VK_NULL_HANDLE)
		return;

    // _device-

	// m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, m_ClearColor._color, 1.0f, 0);

	// // Begin drawing the scene
	// if (SUCCEEDED(m_pD3DDevice->BeginScene()))
	// {
	// 	// TODO : (Optional) 3D Rendering here

	// 	// Draw sprites
	// 	if (SUCCEEDED(m_pD3DSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_DEPTH_FRONTTOBACK)))
	// 	{
	// 		if (m_pCamera)
	// 		{
	// 			D3DXMATRIX viewMat;
	// 			D3DXMatrixIdentity(&viewMat);

	// 			D3DXMATRIX scaleMat;
	// 			D3DXMatrixScaling(&scaleMat, m_pCamera->GetZoom(), m_pCamera->GetZoom(), 1.0f);

	// 			D3DXMATRIX rotationMat;
	// 			D3DXMatrixRotationZ(&rotationMat, m_pCamera->GetRotation());

	// 			D3DXVECTOR2 position = m_pCamera->getPosition() - m_pCamera->GetCenter();

	// 			viewMat._41 = -D3DXVec2Dot(&D3DXVECTOR2(1, 0), &position);
	// 			viewMat._42 = -D3DXVec2Dot(&D3DXVECTOR2(0, 1), &position);

	// 			viewMat = scaleMat * rotationMat * viewMat;

	// 			m_pD3DDevice->SetTransform(D3DTS_VIEW, &viewMat);
	// 		}

	// 		if (!_RenderLists.Empty())
	// 		{
	// 			for (unsigned int i = 0; i < _RenderLists.Size(); i++)
	// 			{
	// 				for (RenderList::iterator o = _RenderLists.At(i)->begin(); o != _RenderLists.At(i)->end(); o++)
	// 				{
	// 					if ((*o)->IsVisible())
	// 					{
	// 						switch ((*o)->getRenderableType())
	// 						{
	// 						case RENDERABLE_TYPE_IMAGE:
	// 							_DrawImage((Image *)(*o));
	// 							break;
	// 						case RENDERABLE_TYPE_ANIMATION:
	// 						{
	// 							Animation *pAnimation = (Animation *)(*o);
	// 							Frame *frame = pAnimation->GetCurrentFrame();

	// 							if (frame)
	// 								_DrawImage(frame->GetSprite(), 0xFFFFFFFF, frame->GetSprite()->GetCenter());
	// 						}
	// 						break;
	// 						}
	// 					}
	// 				}
	// 			}
	// 		}

	// 		m_pD3DSprite->End();
	// 	}

	// 	// TODO : Font rendering here

	// 	m_pD3DDevice->EndScene();
	// }

	// m_pD3DDevice->Present(NULL, NULL, NULL, NULL);
	// InvalidateRect(m_hWnd, NULL, true);
}

ITexture *RendererVK::CreateTexture(const char *szFilename, color colorKey)
{
    ITexture *texture = NULL;

    return NULL;
}
