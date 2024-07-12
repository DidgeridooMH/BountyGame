#include "Otter/Render/Texture/Image.h"

#include "Otter/Render/Memory/MemoryType.h"
#include "Otter/Util/Log.h"

bool image_create(VkExtent2D extents, uint32_t layers, VkFormat format,
    VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice, Image* image)
{
  VkImageCreateInfo imageCreateInfo = {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent = {.width = extents.width, .height = extents.height, .depth = 1},
      .mipLevels     = 1,
      .samples       = VK_SAMPLE_COUNT_1_BIT,
      .arrayLayers   = layers,
      .format        = format,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage         = usage};

  if (vkCreateImage(logicalDevice, &imageCreateInfo, NULL, &image->image)
      != VK_SUCCESS)
  {
    return false;
  }

  image->size   = extents;
  image->format = format;

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(logicalDevice, image->image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {
      .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size};

  if (!memory_type_find(memRequirements.memoryTypeBits, physicalDevice,
          memoryProperties, &allocInfo.memoryTypeIndex))
  {
    LOG_ERROR("Could not find proper memory for the render image.");
    return false;
  }

  if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, &image->memory)
      != VK_SUCCESS)
  {
    LOG_ERROR("Could not allocate memory for the render image.");
    return false;
  }

  if (vkBindImageMemory(logicalDevice, image->image, image->memory, 0)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to bind memory to render image.");
    return false;
  }

  return true;
}

static void image_transition_layout(VkImageLayout oldLayout,
    VkImageLayout newLayout, VkCommandBuffer commandBuffer, Image* image)
{
  VkImageMemoryBarrier barrier = {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout           = oldLayout,
      .newLayout           = newLayout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image               = image->image,
      .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
             .baseMipLevel                = 0,
             .levelCount                  = 1,
             .baseArrayLayer              = 0,
             .layerCount                  = 1},
      .srcAccessMask       = 0,
      .dstAccessMask       = 0};

  VkPipelineStageFlags sourceStage      = 0;
  VkPipelineStageFlags destinationStage = 0;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
      && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
  {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
           && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  else
  {
    LOG_WARNING("Unsupported layout transition.");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, NULL,
      0, NULL, 1, &barrier);
}

void image_upload(GpuBuffer* buffer, VkCommandBuffer commandBuffer,
    VkDevice logicalDevice, Image* image)
{
  image_transition_layout(VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, commandBuffer, image);

  VkBufferImageCopy region = {.bufferOffset = 0,
      .bufferRowLength                      = 0,
      .bufferImageHeight                    = 0,
      .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel                    = 0,
          .baseArrayLayer              = 0,
          .layerCount                  = 1},
      .imageOffset      = {0, 0, 0},
      .imageExtent      = {image->size.width, image->size.height, 1}};

  vkCmdCopyBufferToImage(commandBuffer, buffer->buffer, image->image,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  image_transition_layout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, commandBuffer, image);
}

void image_destroy(Image* image, VkDevice logicalDevice)
{
  if (image->image != VK_NULL_HANDLE)
  {
    LOG_DEBUG("Destroying image %llx", image->image);
    vkDestroyImage(logicalDevice, image->image, NULL);
  }
  if (image->memory != VK_NULL_HANDLE)
  {
    LOG_DEBUG("Freeing memory %llx", image->memory);
    vkFreeMemory(logicalDevice, image->memory, NULL);
  }
}

