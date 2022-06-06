// TextureVK.h

#include "ITexture.h"

#include "RendererVK.h"

class TextureVK: public ITexture
{
    VkImage _image;

public:
    TextureVK(const char *path);
    ~TextureVK();



};