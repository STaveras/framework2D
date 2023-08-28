
#pragma once

#include "Types.h"
#include "Material.h"

#include <array>

typedef struct vertex 
{
    vector3 position;
    vector2 texCoord;
    Color color;

#ifdef VULKAN_H_
    static VkVertexInputBindingDescription getBindingDescription() {
       VkVertexInputBindingDescription bindingDescription{};
       bindingDescription.binding = 0;
       bindingDescription.stride = sizeof(Vertex);
       bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
       return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
       std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

       // Position attribute
       attributeDescriptions[0].binding = 0;
       attributeDescriptions[0].location = 0;
       attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
       attributeDescriptions[0].offset = offsetof(Vertex, position);

       // Texture coordinate attribute
       attributeDescriptions[1].binding = 0;
       attributeDescriptions[1].location = 1;
       attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
       attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

       // Color attribute
       attributeDescriptions[2].binding = 0;
       attributeDescriptions[2].location = 2;
       attributeDescriptions[2].format = VK_FORMAT_R8G8B8A8_UNORM;
       attributeDescriptions[2].offset = offsetof(Vertex, color);

       return attributeDescriptions;
    }
#endif

}Vertex;