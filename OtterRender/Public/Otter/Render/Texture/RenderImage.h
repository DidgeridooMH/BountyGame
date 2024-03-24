#pragma once

#include <vulkan/vulkan.h>

typedef struct RenderImage
{
  VkImage image;
  VkDeviceMemory memory;
  VkExtent2D size;
} RenderImage;

bool render_image_create(VkExtent2D extents, uint32_t layers, VkFormat format,
    VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    RenderImage* renderImage);

void render_image_destroy(RenderImage* image, VkDevice logicalDevice);
