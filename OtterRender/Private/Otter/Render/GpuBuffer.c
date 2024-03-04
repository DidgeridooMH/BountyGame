#include "Otter/Render/GpuBuffer.h"

static bool gpu_buffer_find_memory_type(uint32_t typeFilter,
    VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties,
    uint32_t* memoryIndex)
{
  VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(
      physicalDevice, &physicalMemoryProperties);

  for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
  {
    bool isCorrectType = typeFilter & (1 << i);
    bool hasCorrectProperties =
        (physicalMemoryProperties.memoryTypes[i].propertyFlags & properties)
        == properties;
    if (isCorrectType && hasCorrectProperties)
    {
      *memoryIndex = i;
      return true;
    }
  }

  return false;
}

GpuBuffer* gpu_buffer_allocate(VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags memoryProperties, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  GpuBuffer* buffer = malloc(sizeof(GpuBuffer));
  if (buffer == NULL)
  {
    fprintf(stderr, "OOM\n");
    return NULL;
  }
  buffer->size = size;

  VkBufferCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size  = buffer->size,
      .usage = usage};

  if (vkCreateBuffer(logicalDevice, &createInfo, NULL, &buffer->buffer)
      != VK_SUCCESS)
  {
    free(buffer);
    return NULL;
  }

  VkMemoryRequirements memoryRequirements;
  vkGetBufferMemoryRequirements(
      logicalDevice, buffer->buffer, &memoryRequirements);

  uint32_t memoryIndex;
  if (!gpu_buffer_find_memory_type(memoryRequirements.memoryTypeBits,
          physicalDevice, memoryProperties, &memoryIndex))
  {
    fprintf(stderr, "Failed to find suitable memory type\n");
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    free(buffer);
    return NULL;
  }

  VkMemoryAllocateInfo allocateInfo = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize  = memoryRequirements.size,
      .memoryTypeIndex = memoryIndex};
  if (vkAllocateMemory(logicalDevice, &allocateInfo, NULL, &buffer->memory)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to allocate GPU memory.\n");
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    free(buffer);
    return NULL;
  }

  if (vkBindBufferMemory(logicalDevice, buffer->buffer, buffer->memory, 0)
      != VK_SUCCESS)
  {
    vkFreeMemory(logicalDevice, buffer->memory, NULL);
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    free(buffer);
    return NULL;
  }

  return buffer;
}

void gpu_buffer_free(GpuBuffer* buffer, VkDevice logicalDevice)
{
  vkFreeMemory(logicalDevice, buffer->memory, NULL);
  vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
  free(buffer);
}

bool gpu_buffer_write(GpuBuffer* buffer, uint8_t* data, VkDeviceSize size,
    VkDeviceSize offset, VkDevice logicalDevice)
{
  void* mapping;
  if (vkMapMemory(logicalDevice, buffer->memory, offset, size, 0, &mapping)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to write to buffer.");
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
    printf("WARN: Source and destination buffer sizes do not match\n");
    return;
  }

  VkBufferCopy region = {.srcOffset = 0, .dstOffset = 0, .size = source->size};

  vkCmdCopyBuffer(
      commandBuffer, source->buffer, destination->buffer, 1, &region);
}
