#pragma once

#include "Otter/Render/export.h"
#include <vulkan/vulkan.h>

typedef struct GpuBuffer
{
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
} GpuBuffer;

OTTERRENDER_API bool gpu_buffer_allocate(GpuBuffer* buffer, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

OTTERRENDER_API void gpu_buffer_free(GpuBuffer* buffer, VkDevice logicalDevice);

OTTERRENDER_API bool gpu_buffer_write(GpuBuffer* buffer, uint8_t* data,
    VkDeviceSize size, VkDeviceSize offset, VkDevice logicalDevice);

OTTERRENDER_API void gpu_buffer_transfer(
    GpuBuffer* source, GpuBuffer* destination, VkCommandBuffer commandBuffer);
