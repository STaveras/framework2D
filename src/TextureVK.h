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

    uint32_t _width;
    uint32_t _height;

public:
    TextureVK(const char* path);
    ~TextureVK();

    unsigned int getWidth() const { return _width; }
    unsigned int getHeight() const { return _height; }

    VkImage getImage() const { return _image; }
    VkImageView getImageView() const { return _imageView; }
    VkSampler getSampler() const { return _sampler; }
};
