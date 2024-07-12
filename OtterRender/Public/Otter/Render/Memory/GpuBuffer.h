#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/export.h"

typedef struct GpuBuffer
{
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  void* mapped;
} GpuBuffer;

OTTERRENDER_API bool gpu_buffer_allocate(GpuBuffer* buffer, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

OTTERRENDER_API void gpu_buffer_free(GpuBuffer* buffer, VkDevice logicalDevice);

OTTERRENDER_API bool gpu_buffer_write(GpuBuffer* buffer, const uint8_t* data,
    VkDeviceSize size, VkDeviceSize offset, VkDevice logicalDevice);

OTTERRENDER_API void gpu_buffer_transfer(
    GpuBuffer* source, GpuBuffer* destination, VkCommandBuffer commandBuffer);

OTTERRENDER_API void gpu_buffer_map_all(
    GpuBuffer* buffer, VkDevice logicalDevice);

OTTERRENDER_API void gpu_buffer_unmap(
    GpuBuffer* buffer, VkDevice logicalDevice);
