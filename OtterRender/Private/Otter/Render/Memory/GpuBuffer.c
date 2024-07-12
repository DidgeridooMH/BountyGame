#include "Otter/Render/Memory/GpuBuffer.h"

#include "Otter/Render/Memory/MemoryType.h"
#include "Otter/Util/Log.h"

// TODO: We should really be only allocating one buffer for the MVP, then
// binding different locations for each model.
bool gpu_buffer_allocate(GpuBuffer* buffer, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  buffer->size   = size;
  buffer->mapped = NULL;

  VkBufferCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = buffer->size,
      .usage = usage};

  if (vkCreateBuffer(logicalDevice, &createInfo, NULL, &buffer->buffer)
      != VK_SUCCESS)
  {
    return false;
  }

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(
      logicalDevice, buffer->buffer, &memoryRequirements);

  uint32_t memoryIndex;
  if (!memory_type_find(memoryRequirements.memoryTypeBits, physicalDevice,
          memoryProperties, &memoryIndex))
  {
    LOG_ERROR("Failed to find suitable memory type");
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    return false;
  }

  VkMemoryAllocateInfo allocateInfo = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize  = memoryRequirements.size,
      .memoryTypeIndex = memoryIndex};
  if (vkAllocateMemory(logicalDevice, &allocateInfo, NULL, &buffer->memory)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to allocate GPU memory.");
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    return false;
  }

  if (vkBindBufferMemory(logicalDevice, buffer->buffer, buffer->memory, 0)
      != VK_SUCCESS)
  {
    vkFreeMemory(logicalDevice, buffer->memory, NULL);
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    return false;
  }

  return true;
}

void gpu_buffer_free(GpuBuffer* buffer, VkDevice logicalDevice)
{
  vkFreeMemory(logicalDevice, buffer->memory, NULL);
  vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
}

bool gpu_buffer_write(GpuBuffer* buffer, const uint8_t* data, VkDeviceSize size,
    VkDeviceSize offset, VkDevice logicalDevice)
{
  void* mapping;
  if (vkMapMemory(logicalDevice, buffer->memory, offset, size, 0, &mapping)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to write to buffer.");
    return false;
  }

  memcpy(mapping, data, size);
  vkUnmapMemory(logicalDevice, buffer->memory);

  return true;
}

void gpu_buffer_transfer(
    GpuBuffer* source, GpuBuffer* destination, VkCommandBuffer commandBuffer)
{
  if (source->size != destination->size)
  {
    LOG_WARNING("Source and destination buffer sizes do not match");
    return;
  }

  VkBufferCopy region = {.srcOffset = 0, .dstOffset = 0, .size = source->size};

  vkCmdCopyBuffer(
      commandBuffer, source->buffer, destination->buffer, 1, &region);
}

void gpu_buffer_map_all(GpuBuffer* buffer, VkDevice logicalDevice)
{
  // TODO: Check for error.
  vkMapMemory(
      logicalDevice, buffer->memory, 0, buffer->size, 0, &buffer->mapped);
}

void gpu_buffer_unmap(GpuBuffer* buffer, VkDevice logicalDevice)
{
  vkUnmapMemory(logicalDevice, buffer->memory);
}
