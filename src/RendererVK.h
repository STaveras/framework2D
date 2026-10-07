// RendererVK.h
#pragma once

#include "Renderer.h"

#include "EventSystem.h"
#include "ImageLoaders.h"
#include "Window.h"

#include <optional>
#include <vector>

class Font;
class TextureVK;

class RendererVK : public IRenderer/*, public Window::EventListener*/
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

	std::vector<VkFence> inFlightFences;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;

	std::vector<VkCommandBuffer> _commandBuffers;

	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	VkExtent2D _swapChainExtent;
	VkFormat _swapChainImageFormat;

	// Every texture owns one descriptor set. Pools are added as textures need
	// them, so the number a level can load is not capped by one pool's size.
	std::vector<VkDescriptorPool> _descriptorPools;
	VkDescriptorSetLayout _samplerDescriptorSetLayout;

	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> _swapChainFramebuffers;

	std::vector<VkShaderModule> _shaderModules;

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

	// Sprites and glyphs are queued as quads in world or screen pixels while the
	// render lists are walked, then uploaded and drawn in batches: consecutive
	// quads sharing a texture and a render list go out in one vkCmdDraw.
	struct SpriteVertex { float x, y, u, v; unsigned char r, g, b, a; };
	struct SpriteBatch
	{
		VkDescriptorSet descriptorSet;
		glm::mat4 transform; // Pixels to clip space for the batch's render list
		uint32_t firstVertex;
		uint32_t vertexCount;
	};
	std::vector<SpriteVertex> _spriteVertices;
	std::vector<SpriteBatch> _spriteBatches;
	glm::mat4 _listTransform = glm::mat4(1.0f);
	bool _listStarted = false; // The next quad opens a new batch
	vector2 _viewMin, _viewMax; // World-space bounds of the current list, for culling

	// Host-visible vertex buffer for each frame in flight, grown as needed.
	struct VertexBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		void* mapped = nullptr;
		VkDeviceSize capacity = 0;
	};
	std::vector<VertexBuffer> _vertexBuffers;

	// Font glyphs are untextured; they sample this 1x1 white texture.
	TextureVK* _whiteTexture = nullptr;

	void _queueQuad(VkDescriptorSet descriptorSet, const SpriteVertex (&quad)[4]);
	void _drawImage(Sprite* sprite, Color tint, vector2 offset, bool screenSpace);
	void _drawFont(Font* font, Color tint, vector2 offset);
	void _drawSpriteBatches(VkCommandBuffer commandBuffer);
	void _uploadSpriteVertices(VertexBuffer& vertexBuffer);
	void _destroyVertexBuffer(VertexBuffer& vertexBuffer);
	glm::mat4 _cameraTransform(const vector2& cameraPosition) const;

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	bool isDeviceSuitable(VkPhysicalDevice device);
	void pickPhysicalDevice(VkInstance instance);
	void createLogicalDevice(VkPhysicalDevice physicalDevice);
	void createSwapChain(VkPhysicalDevice physicalDevice, VkDevice device, GLFWwindow* window); // TODO: Swap to "Window"
	void recreateSwapChain(void);
	void cleanupSwapChain(void);

	void createImageViews(VkDevice device);

	void createRenderPass(VkDevice device);

	void createGraphicsPipeline(VkDevice device);
	VkDescriptorPool createDescriptorPool(void);

	void createFramebuffers(VkDevice device);

	void createCommandPool(VkDevice device);
	void createCommandBuffers(VkDevice device);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void createSyncObjects(void); // I don't necessarily want to even declare this here

	void onWindowResized(const Event& e);

	uint32_t currentFrame = 0;

protected:
	void _beginRenderList(const RenderList& renderList) override;
	void _renderSprite(Sprite* sprite, Color tint, const vector2& offset, const RenderList& renderList) override;
	void _renderFont(Font* font, Color tint, const vector2& offset, const RenderList& renderList) override;

public:
	RendererVK(void);

	void initialize(void);
	void shutdown(void);
	void render(void);

	void setVerticalSync(bool vsyncEnabled) override;

	VkDevice getDevice(void) const { return _device; }

	VkCommandBuffer beginSingleTimeCommands(void);
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	ITexture* createTexture(const char* szFilename, Color colorKey = 0);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkSampler createSampler(void);
	// Descriptor set binding a texture's view and sampler; pool receives the pool to free it to.
	VkDescriptorSet allocateTextureDescriptorSet(VkImageView imageView, VkSampler sampler, VkDescriptorPool& pool);
	void freeTextureDescriptorSet(VkDescriptorPool pool, VkDescriptorSet descriptorSet);
};
