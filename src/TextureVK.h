// TextureVK.h

#pragma once

#include "ITexture.h"
#include "RendererVK.h"

class TextureVK : public ITexture
{
    VkImage _image;
    VkDeviceMemory _imageMemory;
    VkImageView _imageView;
    VkSampler _sampler;
    VkDescriptorSet _descriptorSet;
    VkDescriptorPool _descriptorPool; // Pool _descriptorSet was allocated from

    uint32_t _width = 0;
    uint32_t _height = 0;

    // Creates the image, view, sampler and descriptor set from RGBA8 pixels.
    void _upload(const unsigned char* rgbaPixels, uint32_t width, uint32_t height);

public:
    TextureVK(const char* path);
    TextureVK(const char* name, const unsigned char* rgbaPixels, uint32_t width, uint32_t height);
    ~TextureVK();

    unsigned int getWidth() const { return _width; }
    unsigned int getHeight() const { return _height; }

    VkImage getImage() const { return _image; }
    VkImageView getImageView() const { return _imageView; }
    VkSampler getSampler() const { return _sampler; }
    VkDescriptorSet getDescriptorSet() const { return _descriptorSet; }
};
