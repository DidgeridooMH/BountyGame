#include "Otter/Render/Texture/Texture.h"

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Util/Log.h"

bool texture_create(Texture* texture, const uint8_t* data, uint32_t width,
    uint32_t height, uint32_t channels, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice, VkCommandPool commandPool, VkQueue commandQueue)
{
  GpuBuffer transferBuffer = {0};

  if (!gpu_buffer_allocate(&transferBuffer, width * height * channels,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    return false;
  }

  if (!gpu_buffer_write(
          &transferBuffer, data, transferBuffer.size, 0, logicalDevice))
  {
    LOG_ERROR("Failed to write data to transfer buffer");
    gpu_buffer_free(&transferBuffer, logicalDevice);
    return false;
  }

  VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
  switch (channels)
  {
  case 1:
    format = VK_FORMAT_R8_SRGB;
    break;
  case 2:
    format = VK_FORMAT_R8G8_SRGB;
    break;
  case 3:
    format = VK_FORMAT_R8G8B8_SRGB;
    break;
  default:
    break;
  }

  if (!image_create((VkExtent2D){width, height}, 1, format,
          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice,
          &texture->image))
  {
    LOG_ERROR("Failed to create image for texture");
    gpu_buffer_free(&transferBuffer, logicalDevice);
    return false;
  }

  VkCommandBufferAllocateInfo commandBufferAlloc = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
  VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  if (vkAllocateCommandBuffers(
          logicalDevice, &commandBufferAlloc, &commandBuffer)
      != VK_SUCCESS)
  {
    gpu_buffer_free(&transferBuffer, logicalDevice);
    image_destroy(&texture->image, logicalDevice);
    return false;
  }

  VkCommandBufferBeginInfo commandBufferBeginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  if (vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo)
      != VK_SUCCESS)
  {
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    gpu_buffer_free(&transferBuffer, logicalDevice);
    image_destroy(&texture->image, logicalDevice);
    return false;
  }

  image_upload(&transferBuffer, commandBuffer, logicalDevice, &texture->image);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    gpu_buffer_free(&transferBuffer, logicalDevice);
    image_destroy(&texture->image, logicalDevice);
    return false;
  }

  VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount           = 1,
      .pCommandBuffers              = &commandBuffer};
  if (vkQueueSubmit(commandQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
    gpu_buffer_free(&transferBuffer, logicalDevice);
    image_destroy(&texture->image, logicalDevice);
    return false;
  }

  vkQueueWaitIdle(commandQueue);

  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);

  gpu_buffer_free(&transferBuffer, logicalDevice);

  if (!image_sampler_create(&texture->image, logicalDevice, &texture->sampler))
  {
    image_destroy(&texture->image, logicalDevice);
    return false;
  }

  return true;
}

void texture_destroy(Texture* texture, VkDevice logicalDevice)
{
  image_destroy(&texture->image, logicalDevice);
  image_sampler_destroy(&texture->sampler, logicalDevice);
}

