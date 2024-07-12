#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/export.h"

typedef struct Image
{
  VkImage image;
  VkFormat format;
  VkDeviceMemory memory;
  VkExtent2D size;
} Image;

OTTERRENDER_API bool image_create(VkExtent2D extents, uint32_t layers,
    VkFormat format, VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice, Image* image);

OTTERRENDER_API void image_upload(GpuBuffer* buffer,
    VkCommandBuffer commandBuffer, VkDevice logicalDevice, Image* image);

OTTERRENDER_API void image_destroy(Image* image, VkDevice logicalDevice);
