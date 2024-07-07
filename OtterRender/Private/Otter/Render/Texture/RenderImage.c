#include "Otter/Render/Texture/RenderImage.h"

#include "Otter/Render/Memory/MemoryType.h"
#include "Otter/Util/Log.h"

bool render_image_create(VkExtent2D extents, uint32_t layers, VkFormat format,
    VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    RenderImage* renderImage)
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

  if (vkCreateImage(logicalDevice, &imageCreateInfo, NULL, &renderImage->image)
      != VK_SUCCESS)
  {
    return false;
  }

  renderImage->size = extents;

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(
      logicalDevice, renderImage->image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {
      .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size};

  if (!memory_type_find(memRequirements.memoryTypeBits, physicalDevice,
          memoryProperties, &allocInfo.memoryTypeIndex))
  {
    LOG_ERROR("Could not find proper memory for the render image.\n");
    return false;
  }

  if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, &renderImage->memory)
      != VK_SUCCESS)
  {
    LOG_ERROR("Could not allocate memory for the render image.\n");
    return false;
  }

  if (vkBindImageMemory(
          logicalDevice, renderImage->image, renderImage->memory, 0)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to bind memory to render image.\n");
    return false;
  }

  return true;
}

void render_image_destroy(RenderImage* image, VkDevice logicalDevice)
{
  if (image->image != VK_NULL_HANDLE)
  {
    vkDestroyImage(logicalDevice, image->image, NULL);
  }
  if (image->memory != VK_NULL_HANDLE)
  {
    vkFreeMemory(logicalDevice, image->memory, NULL);
  }
}
