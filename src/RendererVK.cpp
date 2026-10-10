// Created:  3/31/2022
// Modified: 4/1/2022

// This is a Vulkan renderer based on the tutorial at: https://vulkan-tutorial.com/

#ifdef _WIN32
#pragma comment(lib, "vulkan-1.lib")
#endif

#include "RendererVK.h"

#include "Engine2D.h"
#include "FileSystem.h"
#include "FramePacer.h"
#include "System.h"
#include "Camera.h"

#include "Animation.h"
#include "Font.h"
#include "Sprite.h"
#include "TextureVK.h"

// NOTE: We should try and decouple the ide of a "window" from this, considering we may want to use this to render off-screen
#include <iostream>
#include <set>

#include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp
#include <filesystem>

#if __APPLE__
#include <mach-o/dyld.h> // _NSGetExecutablePath
#endif

// NOTE: We may need this in the future to directly handle memory
// typedef struct VkAllocationCallbacks {
//     void*                                   pUserData;
//     PFN_vkAllocationFunction                pfnAllocation;
//     PFN_vkReallocationFunction              pfnReallocation;
//     PFN_vkFreeFunction                      pfnFree;
//     PFN_vkInternalAllocationNotification    pfnInternalAllocation;
//     PFN_vkInternalFreeNotification          pfnInternalFree;
// }

// One frame in flight: the CPU never runs ahead of the GPU, so input sampled
// for a frame is not stuck behind older queued frames.
const int MAX_FRAMES_IN_FLIGHT = 1;

// Descriptor sets (one per texture) per pool; more pools are added when one fills.
const uint32_t TEXTURES_PER_DESCRIPTOR_POOL = 256;

VkAllocationCallbacks vkCallbacks{};

const std::vector<const char*> instanceExtensions = {
#if __APPLE__
	VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
	VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#endif
};

const std::vector<const char*> deviceExtensions = {
#if __APPLE__
	// MoltenVK is a portability subset
	VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
#endif
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Set per frame, so the pipeline does not depend on the swap chain size.
std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;

VkDebugUtilsMessengerEXT debugMessenger;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {



	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		std::cerr << std::endl << "Vulkan -- " << pCallbackData->pMessage << std::endl;
	}
	return VK_FALSE;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
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

RendererVK::RendererVK(void)
	: IRenderer(RENDERER_TYPE_VK),
	  _instance(VK_NULL_HANDLE),
	  _device(VK_NULL_HANDLE),
	  _physicalDevice(VK_NULL_HANDLE),
	  _surface(VK_NULL_HANDLE),
	  _presentQueue(VK_NULL_HANDLE),
	  _graphicsQueue(VK_NULL_HANDLE),
	  _graphicsPipeline(VK_NULL_HANDLE),
	  _pipelineLayout(VK_NULL_HANDLE),
	  _renderPass(VK_NULL_HANDLE),
	  _commandPool(VK_NULL_HANDLE),
	  _samplerDescriptorSetLayout(VK_NULL_HANDLE) {
}

// Directory of the running executable, or empty if it cannot be determined.
static std::filesystem::path executableDirectory(void)
{
	namespace fs = std::filesystem;

	fs::path executable;
#if defined(_WIN32)
	wchar_t buffer[MAX_PATH];
	const DWORD length = GetModuleFileNameW(NULL, buffer, MAX_PATH);
	if (length > 0 && length < MAX_PATH) {
		executable = fs::path(std::wstring(buffer, length));
	}
#elif __APPLE__
	uint32_t size = 0;
	_NSGetExecutablePath(nullptr, &size);
	std::string buffer(size, '\0');
	if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
		executable = buffer.c_str();
	}
#else
	std::error_code readError;
	executable = fs::read_symlink("/proc/self/exe", readError);
#endif

	if (executable.empty()) {
		return fs::path();
	}

	std::error_code canonicalError;
	const fs::path resolved = fs::weakly_canonical(executable, canonicalError);
	return (canonicalError ? executable : resolved).parent_path();
}

// Compiled shaders go to bin/cache/shader (utl/compile-shaders.*), next to the
// executable that every build places in bin/. Look there first so the working
// directory does not matter, then next to the data directory, which also sits in bin/.
static std::vector<char> readShader(const char* fileName)
{
	namespace fs = std::filesystem;

	std::vector<fs::path> searchDirectories;

	const fs::path exeDirectory = executableDirectory();
	if (!exeDirectory.empty()) {
		searchDirectories.push_back(exeDirectory / "cache" / "shader");
	}

	std::error_code absoluteError;
	const fs::path dataShaderDirectory = fs::absolute(fs::path(System::GlobalDataPath()).parent_path() / "cache" / "shader", absoluteError).lexically_normal();
	if (std::find(searchDirectories.begin(), searchDirectories.end(), dataShaderDirectory) == searchDirectories.end()) {
		searchDirectories.push_back(dataShaderDirectory);
	}

	std::string searched;
	for (const fs::path& directory : searchDirectories) {
		const fs::path shaderPath = directory / fileName;
		std::vector<char> code = FileSystem::File::Read(shaderPath.string());
		if (!code.empty()) {
			return code;
		}
		searched += "\n\t" + shaderPath.string();
	}

	throw std::runtime_error("Vulkan: could not find shader " + std::string(fileName) + ", looked for:" + searched);
}

bool checkValidationLayerSupport(std::vector<const char*>  validationLayers) {

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
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
	for (const auto& queueFamily : queueFamilies) {

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
	// Texels and colors are written as-is, like the OpenGL backend's default
	// framebuffer; an sRGB swap chain would re-encode them and wash them out.
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	// all else fails?
	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool verticalSync = false) {

	// FIFO is the only mode the spec guarantees, and the only one that waits for vblank.
	if (verticalSync)
		return VK_PRESENT_MODE_FIFO_KHR;

	// Without vsync prefer IMMEDIATE (lowest latency, may tear), then MAILBOX (no tearing).
	for (VkPresentModeKHR mode : { VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR }) {
		if (std::find(availablePresentModes.begin(), availablePresentModes.end(), mode) != availablePresentModes.end())
			return mode;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D currentExtent(const VkSurfaceCapabilitiesKHR& capabilities) {

	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() ||
		capabilities.currentExtent.height != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}

	return VkExtent2D();
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {

	int width, height; glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height) };

	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

	return actualExtent;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device) {

	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	if (enableValidationLayers) {
		std::cout << "Available device extensions:" << std::endl;
		for (auto extension : availableExtensions) {
			std::cout << '\t' << extension.extensionName << std::endl;
		}
	}

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions) {
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

	// // collidesWith if the best candidate is suitable at all
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

	for (const auto& device : devices) {
		if (device != VK_NULL_HANDLE) {
			if (isDeviceSuitable(device)) {
				_physicalDevice = device;
				break;
			}
		}
	}
}

void RendererVK::_queueQuad(VkDescriptorSet descriptorSet, const SpriteVertex (&quad)[4])
{
	if (_listStarted || _spriteBatches.empty() || _spriteBatches.back().descriptorSet != descriptorSet) {
		_spriteBatches.push_back({ descriptorSet, _listTransform, static_cast<uint32_t>(_spriteVertices.size()), 0 });
		_listStarted = false;
	}

	// Corners run clockwise from the top left; split into two triangles.
	for (int corner : { 0, 1, 2, 2, 3, 0 }) {
		_spriteVertices.push_back(quad[corner]);
	}
	_spriteBatches.back().vertexCount += 6;
}

void RendererVK::_drawImage(Sprite* sprite, Color tint, vector2 offset, bool screenSpace)
{
	if (!sprite) {
		return;
	}

	TextureVK* texture = static_cast<TextureVK*>(const_cast<ITexture*>(sprite->getTexture()));
	if (!texture) {
		return;
	}

	const RECT& srcRect = sprite->getSrcRect();
	const float srcWidth = static_cast<float>(srcRect.right - srcRect.left);
	const float srcHeight = static_cast<float>(srcRect.bottom - srcRect.top);

	if (srcWidth <= 0.0f || srcHeight <= 0.0f) {
		return;
	}

	const float texWidth = static_cast<float>(texture->getWidth());
	const float texHeight = static_cast<float>(texture->getHeight());
	if (texWidth <= 0.0f || texHeight <= 0.0f) {
		return;
	}

	// Same texture coordinates as RendererGL: sub-rectangles of a sheet are inset
	// by half a texel so neighbouring cells do not bleed in.
	const bool isSubRect =
		srcRect.left > 0 ||
		srcRect.top > 0 ||
		srcRect.right < static_cast<int>(texWidth) ||
		srcRect.bottom < static_cast<int>(texHeight);

	float uInset = 0.0f;
	float vInset = 0.0f;
	if (isSubRect && srcWidth > 1.0f && srcHeight > 1.0f) {
		uInset = 0.5f / texWidth;
		vInset = 0.5f / texHeight;
	}

	const float u0 = (srcRect.left / texWidth) + uInset;
	const float v0 = (srcRect.top / texHeight) + vInset;
	const float u1 = (srcRect.right / texWidth) - uInset;
	const float v1 = (srcRect.bottom / texHeight) - vInset;

	const vector2 position = sprite->getPosition() + offset;
	const vector2 center = sprite->getCenter();
	const vector2 scale = sprite->getScale();
	const float rotationRadians = sprite->getRotation();

	// Transform on the CPU so sprites sharing a texture share one draw call.
	const float c = std::cos(rotationRadians), sn = std::sin(rotationRadians);
	const float xs[4] = { -center.x, srcWidth - center.x, srcWidth - center.x, -center.x };
	const float ys[4] = { -center.y, -center.y, srcHeight - center.y, srcHeight - center.y };
	const float us[4] = { u0, u1, u1, u0 }, vs[4] = { v0, v0, v1, v1 };
	SpriteVertex quad[4];
	vector2 lo(INFINITY, INFINITY), hi(-INFINITY, -INFINITY);
	for (int i = 0; i < 4; ++i) {
		const float x = xs[i] * scale.x, y = ys[i] * scale.y;
		quad[i] = { position.x + c * x - sn * y, position.y + sn * x + c * y,
			us[i], vs[i], tint.r, tint.g, tint.b, tint.a };
		lo.x = std::min(lo.x, quad[i].x); lo.y = std::min(lo.y, quad[i].y);
		hi.x = std::max(hi.x, quad[i].x); hi.y = std::max(hi.y, quad[i].y);
	}
	if (!screenSpace && (hi.x < _viewMin.x || lo.x > _viewMax.x || hi.y < _viewMin.y || lo.y > _viewMax.y)) {
		return;
	}

	_queueQuad(texture->getDescriptorSet(), quad);
}

void RendererVK::_drawFont(Font* font, Color tint, vector2 offset)
{
	if (!font || !_whiteTexture) {
		return;
	}

	const std::string& text = font->getText();
	if (text.empty()) {
		return;
	}

	const int fontHeight = font->getHeight();
	if (fontHeight <= 0) {
		return;
	}

	const vector2 position = font->getPosition() + offset;
	const vector2 center = font->getCenter();
	const vector2 scale = font->getScale();
	const float rotationRadians = font->getRotation();
	const float c = std::cos(rotationRadians), sn = std::sin(rotationRadians);

	// Glyph bitmaps are drawn one quad per set bit, laid out like RendererGL's.
	const auto toVertex = [&](float x, float y) -> SpriteVertex {
		x *= scale.x;
		y *= scale.y;
		return { position.x + c * x - sn * y, position.y + sn * x + c * y,
			0.5f, 0.5f, tint.r, tint.g, tint.b, tint.a };
	};

	float penX = -center.x;
	float penY = -center.y;
	const float lineAdvance = static_cast<float>(fontHeight + 1);

	for (char ch : text) {
		if (ch == '\n') {
			penX = -center.x;
			penY += lineAdvance;
			continue;
		}

		const std::vector<int>& bitmap = font->getBitmap(ch);
		const int glyphWidth = std::max(font->getWidth(ch), 0);
		const int glyphAdvance = std::max(font->getBitmapWidth(), 1);
		for (int rowIndex = 0; rowIndex < static_cast<int>(bitmap.size()); ++rowIndex) {
			const int rowBits = bitmap[rowIndex];
			for (int column = 0; column < glyphWidth; ++column) {
				if (((rowBits >> column) & 1) == 0) {
					continue;
				}

				const float left = penX + static_cast<float>(column);
				const float top = penY + static_cast<float>(rowIndex);
				const SpriteVertex quad[4] = {
					toVertex(left, top),
					toVertex(left + 1.0f, top),
					toVertex(left + 1.0f, top + 1.0f),
					toVertex(left, top + 1.0f)
				};
				_queueQuad(_whiteTexture->getDescriptorSet(), quad);
			}
		}

		penX += static_cast<float>(glyphAdvance + 1);
	}
}

glm::mat4 RendererVK::_cameraTransform(const vector2& cameraPosition) const
{
	// The transform RendererGL builds with glTranslate/glRotate/glScale.
	const vector2 cameraCenter = m_pCamera->getCenter();
	const float zoom = (m_pCamera->getZoom() > 0.0f) ? m_pCamera->getZoom() : 1.0f;
	const float rotation = m_pCamera->getRotation();
	const glm::vec3 zAxis(0.0f, 0.0f, 1.0f);

	glm::mat4 transform(1.0f);
	if (m_pCamera->getZoomAnchorMode() == Camera::ZoomAnchorMode::TargetCenter) {
		transform = glm::translate(transform, glm::vec3(cameraCenter.x, cameraCenter.y, 0.0f));
		transform = glm::rotate(transform, rotation, zAxis);
		transform = glm::scale(transform, glm::vec3(zoom, zoom, 1.0f));
		transform = glm::translate(transform, glm::vec3(-cameraPosition.x, -cameraPosition.y, 0.0f));
	}
	else {
		const vector2 legacyCameraOffset = cameraPosition - cameraCenter;
		transform = glm::scale(transform, glm::vec3(zoom, zoom, 1.0f));
		transform = glm::rotate(transform, rotation, zAxis);
		transform = glm::translate(transform, glm::vec3(-legacyCameraOffset.x, -legacyCameraOffset.y, 0.0f));
	}
	return transform;
}

void RendererVK::_beginRenderList(const RenderList& renderList)
{
	// Game pixels to clip space. Vulkan's clip-space y points down, as screen y does.
	const glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(m_nWidth), 0.0f, static_cast<float>(m_nHeight));

	_listTransform = projection;
	_listStarted = true;

	if (renderList.screenSpace) {
		return;
	}

	if (m_pCamera) {
		const vector2 cameraPosition = _parallaxCameraPosition(renderList);
		_listTransform = projection * _cameraTransform(cameraPosition);
		_viewBounds(cameraPosition, _viewMin, _viewMax);
	}
	else {
		_viewMin = vector2(-INFINITY, -INFINITY);
		_viewMax = vector2(INFINITY, INFINITY);
	}
}

void RendererVK::_renderSprite(Sprite* sprite, Color tint, const vector2& offset, const RenderList& renderList)
{
	_drawImage(sprite, tint, offset, renderList.screenSpace);
}

void RendererVK::_renderFont(Font* font, Color tint, const vector2& offset, const RenderList& renderList)
{
	_drawFont(font, tint, offset);
}

void RendererVK::_uploadSpriteVertices(VertexBuffer& vertexBuffer)
{
	const VkDeviceSize size = sizeof(SpriteVertex) * _spriteVertices.size();

	// This frame's fence has signalled, so the GPU is done with the old buffer.
	if (size > vertexBuffer.capacity) {
		const VkDeviceSize capacity = std::max(size, vertexBuffer.capacity * 2);
		_destroyVertexBuffer(vertexBuffer);

		createBuffer(capacity, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			vertexBuffer.buffer, vertexBuffer.memory);
		vkMapMemory(_device, vertexBuffer.memory, 0, VK_WHOLE_SIZE, 0, &vertexBuffer.mapped);
		vertexBuffer.capacity = capacity;
	}

	memcpy(vertexBuffer.mapped, _spriteVertices.data(), static_cast<size_t>(size));
}

void RendererVK::_destroyVertexBuffer(VertexBuffer& vertexBuffer)
{
	if (vertexBuffer.memory != VK_NULL_HANDLE) {
		vkUnmapMemory(_device, vertexBuffer.memory);
		vkFreeMemory(_device, vertexBuffer.memory, nullptr);
	}
	if (vertexBuffer.buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(_device, vertexBuffer.buffer, nullptr);
	}
	vertexBuffer = VertexBuffer();
}

void RendererVK::_drawSpriteBatches(VkCommandBuffer commandBuffer)
{
	if (_spriteVertices.empty()) {
		return;
	}

	VertexBuffer& vertexBuffer = _vertexBuffers[currentFrame];
	_uploadSpriteVertices(vertexBuffer);

	const VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer.buffer, &offset);

	VkDescriptorSet boundDescriptorSet = VK_NULL_HANDLE;
	const glm::mat4* pushedTransform = nullptr;

	for (const SpriteBatch& batch : _spriteBatches) {
		if (batch.descriptorSet != boundDescriptorSet) {
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &batch.descriptorSet, 0, nullptr);
			boundDescriptorSet = batch.descriptorSet;
		}
		if (!pushedTransform || *pushedTransform != batch.transform) {
			vkCmdPushConstants(commandBuffer, _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &batch.transform);
			pushedTransform = &batch.transform;
		}
		vkCmdDraw(commandBuffer, batch.vertexCount, 1, batch.firstVertex, 0);
	}
}

uint32_t RendererVK::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
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
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

void RendererVK::createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow* window)
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes, m_bVerticalSync);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);

	// Fewest images the surface allows: every extra image is another frame FIFO can queue ahead of the display.
	uint32_t imageCount = std::max(2u, swapChainSupport.capabilities.minImageCount);

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

	// You need to pass the previous swap chain to the oldSwapChain field in the VkSwapchainCreateInfoKHR
	// struct and destroy the old swap chain as soon as you've finished using it.

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

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

	// we might have to wait a lil bit for this to work? (grabbing a "swapChain" apparently takes a while)
	if (_swapChain != VK_NULL_HANDLE)
	{
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
		_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());
	}
}

void RendererVK::createImageViews(VkDevice device)
{
	swapChainImageViews.resize(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {

		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = _swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = _swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

void RendererVK::createGraphicsPipeline(VkDevice device) {

	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(SpriteVertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Position and texture coordinate in floats, tint as normalized bytes
	VkVertexInputAttributeDescription attributeDescriptions[3]{};
	attributeDescriptions[0] = { 0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpriteVertex, x) };
	attributeDescriptions[1] = { 1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(SpriteVertex, u) };
	attributeDescriptions[2] = { 2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(SpriteVertex, r) };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = COUNT_OF(attributeDescriptions);
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;

	auto vertShaderCode = readShader("tri.v.spv");
	auto fragShaderCode = readShader("tri.f.spv");

	VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

	_shaderModules.push_back(vertShaderModule);
	_shaderModules.push_back(fragShaderModule);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport and scissor are dynamic state, set when each frame is recorded.
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE; // Mirrored sprites (negative scale) flip winding
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	// multisampling.pSampleMask = nullptr; // Optional
	// multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	// multisampling.alphaToOneEnable = VK_FALSE; // Optional

	// Not in use.
	// VkPipelineDepthStencilStateCreateInfo depthStencil{};
	// depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	// depthStencil.depthTestEnable = VK_TRUE;
	// depthStencil.depthWriteEnable = VK_TRUE;
	// depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	// depthStencil.depthBoundsTestEnable = VK_FALSE;
	// depthStencil.stencilTestEnable = VK_FALSE;
	// depthStencil.front = {}; // Optional
	// depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Alpha blending, as glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) in RendererGL
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	// colorBlending.blendConstants[0] = 0.0f; // Optional
	// colorBlending.blendConstants[1] = 0.0f; // Optional
	// colorBlending.blendConstants[2] = 0.0f; // Optional
	// colorBlending.blendConstants[3] = 0.0f; // Optional

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	// Define the layout for a combined image sampler
	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 0; // Binding number in the shader (consistent with fragment stage)
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; // Accessible from fragment stage
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; // Expected type by shader

	// Create the descriptor set layout with the defined binding
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &samplerLayoutBinding;

	if (vkCreateDescriptorSetLayout(_device, &layoutInfo, nullptr, &_samplerDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}

	// The vertex shader's transform (pixels to clip space) changes per render list.
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);

	// Set 0 is the texture being drawn
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &_samplerDescriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = COUNT_OF(shaderStages);
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = _renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

VkDescriptorPool RendererVK::createDescriptorPool(void)
{
	// One combined image sampler per set: each set is one texture.
	VkDescriptorPoolSize poolSize{};
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSize.descriptorCount = TEXTURES_PER_DESCRIPTOR_POOL;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // Textures free their own set
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;
	poolInfo.maxSets = TEXTURES_PER_DESCRIPTOR_POOL;

	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}

	_descriptorPools.push_back(descriptorPool);
	return descriptorPool;
}

VkDescriptorSet RendererVK::allocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler, VkDescriptorPool& pool)
{
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_samplerDescriptorSetLayout;

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

	// Use the newest pool; when it is full, add another and try once more.
	pool = _descriptorPools.empty() ? createDescriptorPool() : _descriptorPools.back();
	allocInfo.descriptorPool = pool;

	if (vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		pool = createDescriptorPool();
		allocInfo.descriptorPool = pool;

		if (vkAllocateDescriptorSets(_device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate descriptor sets!");
		}
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = descriptorSet;
	descriptorWrite.dstBinding = 0; // Binding number in the shader
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);

	return descriptorSet;
}

void RendererVK::freeTextureDescriptorSet(VkDescriptorPool pool, VkDescriptorSet descriptorSet)
{
	if (_device != VK_NULL_HANDLE && pool != VK_NULL_HANDLE && descriptorSet != VK_NULL_HANDLE) {
		vkFreeDescriptorSets(_device, pool, 1, &descriptorSet);
	}
}

void RendererVK::createRenderPass(VkDevice device)
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = _swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT /*| VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT8*/;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void RendererVK::createFramebuffers(VkDevice device)
{
	_swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {

		VkImageView attachments[] = {
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = _swapChainExtent.width;
		framebufferInfo.height = _swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void RendererVK::createCommandPool(VkDevice device)
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

void RendererVK::createCommandBuffers(VkDevice device)
{
	_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT); //_swapChainFramebuffers.size()

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VkShaderModule RendererVK::createShaderModule(VkDevice device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

// This is going to be equivalent to the DirectX render implementation.
void RendererVK::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;                  // Optional
	beginInfo.pInheritanceInfo = nullptr; // Optional

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _renderPass;
	renderPassInfo.framebuffer = _swapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _swapChainExtent;

	VkClearValue clearColor = { {{m_ClearColor.r / 255.0f, m_ClearColor.g / 255.0f, m_ClearColor.b / 255.0f, m_ClearColor.a / 255.0f}} };
	renderPassInfo.pClearValues = &clearColor;
	renderPassInfo.clearValueCount = 1;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

	// The whole framebuffer; the projection maps the game resolution onto it.
	VkViewport viewport{};
	viewport.width = static_cast<float>(_swapChainExtent.width);
	viewport.height = static_cast<float>(_swapChainExtent.height);
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent = _swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Queue world-space lists first, then screen-space, matching the other backends.
	_spriteVertices.clear();
	_spriteBatches.clear();
	_drawRenderLists(false);
	_drawRenderLists(true);
	_drawSpriteBatches(commandBuffer);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}
}

void RendererVK::cleanupSwapChain(void)
{
	if (_swapChain != VK_NULL_HANDLE) {

		for (auto framebuffer : _swapChainFramebuffers) {
			vkDestroyFramebuffer(_device, framebuffer, nullptr);
		}

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(_device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(_device, _swapChain, nullptr);
		_swapChain = VK_NULL_HANDLE;
	}

	_swapChainFramebuffers.clear();
	swapChainImageViews.clear();
	_swapChainImages.clear();
}

void RendererVK::setVerticalSync(bool vsyncEnabled)
{
	const bool changed = vsyncEnabled != m_bVerticalSync;
	IRenderer::setVerticalSync(vsyncEnabled);

	// The present mode is baked into the swap chain, so rebuild it if one exists.
	if (changed && _swapChain != VK_NULL_HANDLE)
		recreateSwapChain();
}

void RendererVK::recreateSwapChain(void)
{
	vkDeviceWaitIdle(_device);

	cleanupSwapChain();

	createSwapChain(_physicalDevice, _device, Renderer::mainWindow->getUnderlyingWindow());
	createImageViews(_device);
	createFramebuffers(_device);
}

void RendererVK::createSyncObjects(void)
{
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(_device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("failed to create sync objects!");
		}
	}
}

void RendererVK::onWindowResized(const Event& e)
{
	GLFWwindow* window = (GLFWwindow*)e.getSender();

	if (window)
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		// This is not the right way to handle this, especially when we network this engine
		// We'll need to spawn a render thread seperately from the rest of the framework

		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(this->_device);
	}

	recreateSwapChain();
}

void RendererVK::initialize(void)
{
	// std::vector<vertex> _vertices = {{{0.0f, 0.0f, 0.0f}, {1.0f, 0.5f, 0.0f}}};

	// Vertex vertex{{0,0,0},{0,0,0}};

	if (Renderer::mainWindow) {

		GLFWwindow* window = Renderer::mainWindow->getUnderlyingWindow();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = Renderer::mainWindow->getWindowTitle();
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = Renderer::mainWindow->getWindowClassName();
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.pApplicationInfo = &appInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#if __APPLE__
		createInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		for (const char* extension : instanceExtensions) {
			extensions.push_back(extension);
		}

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

		if (enableValidationLayers) {
#if !defined(NDEBUG)
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
			populateDebugMessengerCreateInfo(debugCreateInfo);
#endif
			if (!checkValidationLayerSupport(validationLayers)) {
				throw std::runtime_error("Validation layers requested, but not available!");
			}
			else {
				createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
				createInfo.ppEnabledLayerNames = validationLayers.data();
#if _DEBUG
				createInfo.pNext = &debugCreateInfo;
#endif
			}
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		switch (vkCreateInstance(&createInfo, nullptr, &_instance))
		{
		case VK_SUCCESS:
			std::cout << "Vulkan initialized" << std::endl;
			std::cout << "Requested instance extensions:" << std::endl;

			for (auto instanceExtension : extensions) {
				std::cout << '\t' << instanceExtension << std::endl;
			}
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

		if (enableValidationLayers) {

			std::cout << "Available instance extensions:\n";

			for (const auto& extension : extensionProperties) {
				std::cout << '\t' << extension.extensionName << '\n';
			}
#if !defined(NDEBUG)
			setupDebugMessenger(_instance);
#endif
		}

		pickPhysicalDevice(_instance);

		if (_physicalDevice) {
			createLogicalDevice(_physicalDevice);
			createSwapChain(_physicalDevice, _device, window);
			createImageViews(_device);
			createRenderPass(_device);
			createGraphicsPipeline(_device);
			createFramebuffers(_device);
			createCommandPool(_device);
			createCommandBuffers(_device);
			createSyncObjects();
			createDescriptorPool();

			_vertexBuffers.resize(MAX_FRAMES_IN_FLIGHT);

			const unsigned char white[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
			_whiteTexture = new TextureVK("vulkan:white", white, 1, 1);
		}
	}
}

void RendererVK::shutdown(void)
{
	if (_instance) {

		if (_device)
		{
			vkDeviceWaitIdle(_device);

			for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

				vkDestroySemaphore(_device, imageAvailableSemaphores[i], nullptr);
				vkDestroySemaphore(_device, renderFinishedSemaphores[i], nullptr);
				vkDestroyFence(_device, inFlightFences[i], nullptr);
			}

			// Textures hold device objects and descriptor sets, so release them while the device exists.
			m_Textures.clear();
			delete _whiteTexture;
			_whiteTexture = nullptr;

			for (VertexBuffer& vertexBuffer : _vertexBuffers) {
				_destroyVertexBuffer(vertexBuffer);
			}
			_vertexBuffers.clear();

			for (VkDescriptorPool descriptorPool : _descriptorPools) {
				vkDestroyDescriptorPool(_device, descriptorPool, nullptr);
			}
			_descriptorPools.clear();

			vkDestroyDescriptorSetLayout(_device, _samplerDescriptorSetLayout, nullptr);
			_samplerDescriptorSetLayout = VK_NULL_HANDLE;

			vkDestroyCommandPool(_device, _commandPool, nullptr);

			cleanupSwapChain();

			vkDestroyPipeline(_device, _graphicsPipeline, nullptr);

			vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);

			vkDestroyRenderPass(_device, _renderPass, nullptr);

			for (auto shaderModule : _shaderModules) {
				vkDestroyShaderModule(_device, shaderModule, nullptr);
			}
			_shaderModules.clear();

			if (_device != VK_NULL_HANDLE) {
				vkDestroyDevice(_device, nullptr);

				_device = VK_NULL_HANDLE;
			}
		}

		if (_surface != VK_NULL_HANDLE) {
			vkDestroySurfaceKHR(_instance, _surface, nullptr);
		}

		if (enableValidationLayers) {
#if !defined(NDEBUG)
			destroyDebugUtilsMessengerEXT(_instance, nullptr);
#endif
		}

		vkDestroyInstance(_instance, nullptr);

		_instance = VK_NULL_HANDLE;
	}
}

void RendererVK::render(void)
{
	IRenderer::render();

	if (_device == VK_NULL_HANDLE)
		return;

	vkWaitForFences(_device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;

	VkResult result = vkAcquireNextImageKHR(_device,
		_swapChain,
		UINT64_MAX,
		imageAvailableSemaphores[currentFrame],
		VK_NULL_HANDLE,
		&imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
		return recreateSwapChain();
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	vkResetFences(_device, 1, &inFlightFences[currentFrame]);

	vkResetCommandBuffer(_commandBuffers[currentFrame], 0);

	recordCommandBuffer(_commandBuffers[currentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffers[currentFrame];

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	const VkFence submittedFence = inFlightFences[currentFrame];
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	FramePacer::presentBegin();
	result = vkQueuePresentKHR(_presentQueue, &presentInfo);

	// Finish this frame before returning so the next input poll lands on an idle GPU.
	vkWaitForFences(_device, 1, &submittedFence, VK_TRUE, UINT64_MAX);
	FramePacer::presentEnd();

	switch (result)
	{
	case VK_SUBOPTIMAL_KHR:
	case VK_ERROR_OUT_OF_DATE_KHR:
		return recreateSwapChain();
	case VK_SUCCESS:
		break;
	default: // I feel like this is p. dumb
		throw std::runtime_error("failed to present swap chain image!");
	};
}

VkCommandBuffer RendererVK::beginSingleTimeCommands(void)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool; // Your command pool
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void RendererVK::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE); // Your graphics queue
	vkQueueWaitIdle(_graphicsQueue);

	vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void RendererVK::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	// Define source and destination access masks based on layouts
	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0, // Dependency flags
		0, nullptr, // Memory barriers
		0, nullptr, // Buffer memory barriers
		1, &barrier // Image memory barriers
	);

	endSingleTimeCommands(commandBuffer);
}

void RendererVK::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0; // Specifying 0 means tightly packed data
	region.bufferImageHeight = 0; // Specifying 0 means tightly packed data
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		 width,
		 height,
		 1
	};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // The image must be in this layout for the copy operation
		1,
		&region
	);

	endSingleTimeCommands(commandBuffer);
}

void RendererVK::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	// 1) Validate inputs early
	if (size == 0) {
#if _DEBUG
		std::cerr << "[VK] createBuffer: refusing to create buffer with size=0\n";
#endif
		throw std::runtime_error("createBuffer: size must be > 0");
	}
	if (usage == 0) {
#if _DEBUG
		std::cerr << "[VK] createBuffer: refusing to create buffer with usage=0\n";
#endif
		throw std::runtime_error("createBuffer: usage must not be 0");
	}

	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	// If you only use one graphics queue, EXCLUSIVE is ideal.
	// If you use both graphics and transfer queues concurrently, set CONCURRENT and provide families.
	QueueFamilyIndices q = findQueueFamilies(_physicalDevice);
	const uint32_t fams[2] = { q.graphicsFamily.value(), q.presentFamily.value() };
	bool familiesDiffer = q.graphicsFamily != q.presentFamily;

	if (familiesDiffer) {
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
		bufferInfo.queueFamilyIndexCount = 2;
		bufferInfo.pQueueFamilyIndices = fams;
	}
	else {
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	if (VkResult rc = vkCreateBuffer(_device, &bufferInfo, nullptr, &buffer)) {
		if (rc != VK_SUCCESS) {
#if _DEBUG
			std::cerr << "[VK] vkCreateBuffer failed. size=" << (uint64_t)size
				<< " usage=0x" << std::hex << usage << std::dec
				<< " sharing=" << (familiesDiffer ? "CONCURRENT" : "EXCLUSIVE")
				<< " rc=" << (int)rc << "\n";
#endif
			throw std::runtime_error("Failed to create buffer");
		}
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

	// Align up to required alignment
	VkDeviceSize allocSize = (size + memRequirements.alignment - 1) & ~(memRequirements.alignment - 1);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = allocSize;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

void RendererVK::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(_device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image memory!");
	}

	vkBindImageMemory(_device, image, imageMemory, 0);
}

VkImageView RendererVK::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture image view!");
	}

	return imageView;
}

VkSampler RendererVK::createSampler(void)
{
	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	// Nearest and clamped like TextureGL, so pixel art stays crisp and sheet edges do not wrap.
	// Anisotropy stays off: the samplerAnisotropy device feature is not enabled.
	samplerInfo.magFilter = VK_FILTER_NEAREST;
	samplerInfo.minFilter = VK_FILTER_NEAREST;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 1.0f;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	VkSampler sampler;
	if (vkCreateSampler(_device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}

	return sampler;
}

// TODO: Complete this function to correctly create a texture in Vulkan
ITexture* RendererVK::createTexture(const char* szFilename, Color colorKey)
{

	ITexture* pTexture = _textureExists(szFilename, colorKey);

	if (!pTexture)
	{
		pTexture = (ITexture*)new TextureVK(szFilename, colorKey);
		pTexture->setKeyColor(colorKey);

		// Cache it like the other backends, so sprites sharing an image share one texture.
		m_Textures.store(pTexture);

		//D3DXCreateTextureFromFileEx(
		//	m_pD3DDevice, szFilename,
		//	D3DX_DEFAULT_NONPOW2,
		//	D3DX_DEFAULT_NONPOW2,
		//	D3DX_DEFAULT, 0,
		//	D3DFMT_A8R8G8B8,
		//	D3DPOOL_MANAGED,
		//	D3DX_FILTER_POINT,
		//	D3DX_DEFAULT,
		//	(DWORD)colorKey._color,
		//	&((TextureD3D*)pTexture)->_imageInfo, NULL,
		//	&((TextureD3D*)pTexture)->_texture);
	}
	else {
		//((TextureD3D*)pTexture)->getTexture()->AddRef();
	}

	return pTexture;
}

// Stan Taveras
