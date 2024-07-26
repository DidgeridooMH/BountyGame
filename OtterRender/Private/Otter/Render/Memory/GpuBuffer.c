#include "Otter/Render/Memory/GpuBuffer.h"

#include "Otter/Render/Memory/MemoryType.h"
#include "Otter/Util/Log.h"

static size_t g_allocations = 0;

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
    LOG_ERROR("Failed to create buffer of size %lu", size);
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

  VkMemoryAllocateFlagsInfo allocateFlags = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
      .flags = usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                 ? VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
                 : 0,
  };
  VkMemoryAllocateInfo allocateInfo = {
      .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext           = &allocateFlags,
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
    LOG_ERROR("Unable to bind buffer memory.");
    vkFreeMemory(logicalDevice, buffer->memory, NULL);
    vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);
    return false;
  }

  LOG_DEBUG("Allocation %lu: %lu bytes", g_allocations++, size);

  return true;
}

void gpu_buffer_free(GpuBuffer* buffer, VkDevice logicalDevice)
{
  vkFreeMemory(logicalDevice, buffer->memory, NULL);
  vkDestroyBuffer(logicalDevice, buffer->buffer, NULL);

  g_allocations -= 1;
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

VkDeviceAddress gpu_buffer_get_device_address(
    GpuBuffer* buffer, VkDevice logicalDevice)
{
  return vkGetBufferDeviceAddress(
      logicalDevice, &(VkBufferDeviceAddressInfo){
                         .sType  = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
                         .buffer = buffer->buffer});
}

