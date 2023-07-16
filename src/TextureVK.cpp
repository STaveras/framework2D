// TextureVK.cpp

#include "TextureVK.h"
#include "RendererVK.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

RendererVK* rendererVK = (RendererVK*)Renderer::get();

TextureVK::TextureVK(const char* path) : ITexture(path), _image(VK_NULL_HANDLE), _imageMemory(VK_NULL_HANDLE), _imageView(VK_NULL_HANDLE), _sampler(VK_NULL_HANDLE)
{
   return;

   // Load image using stb_image library
   int texWidth, texHeight, texChannels;
   stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
   VkDeviceSize imageSize = texWidth * texHeight * 4;

   if (!pixels)
   {
      throw std::runtime_error("Failed to load texture image!");
   }

   _width = static_cast<uint32_t>(texWidth);
   _height = static_cast<uint32_t>(texHeight);

   // Create staging buffer
   VkBuffer stagingBuffer;
   VkDeviceMemory stagingBufferMemory;
   rendererVK->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

   // Copy image data to staging buffer
   void* data;
   vkMapMemory(rendererVK->getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
   memcpy(data, pixels, static_cast<size_t>(imageSize));
   vkUnmapMemory(rendererVK->getDevice(), stagingBufferMemory);

   // Free the original image data
   stbi_image_free(pixels);

   // Create image resource
   rendererVK->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _image, _imageMemory);

   //// Transition image layout
   //RendererVK::TransitionImageLayout(_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
   //RendererVK::CopyBufferToImage(stagingBuffer, _image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

   //// Transition image layout for shader access
   //RendererVK::TransitionImageLayout(_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

   //// Destroy staging buffer
   //vkDestroyBuffer(RendererVK::getDevice(), stagingBuffer, nullptr);
   //vkFreeMemory(RendererVK::getDevice(), stagingBufferMemory, nullptr);

   //// Create image view
   //_imageView = RendererVK::CreateImageView(_image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

   //// Create sampler
   //_sampler = RendererVK::CreateSampler();
}

TextureVK::~TextureVK()
{
   vkDestroySampler(rendererVK->getDevice(), _sampler, nullptr);
   vkDestroyImageView(rendererVK->getDevice(), _imageView, nullptr);
   vkDestroyImage(rendererVK->getDevice(), _image, nullptr);
   vkFreeMemory(rendererVK->getDevice(), _imageMemory, nullptr);
}
