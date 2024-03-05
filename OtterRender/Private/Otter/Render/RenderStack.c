#include "Otter/Render/RenderStack.h"

#include "Otter/Render/Memory/MemoryType.h"

static bool render_stack_create_render_image(VkExtent2D extents,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    RenderImage* renderImage)
{
  VkImageCreateInfo gBufferImageCreateInfo = {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent = {.width = extents.width, .height = extents.height, .depth = 1},
      .mipLevels     = 1,
      .arrayLayers   = NUM_OF_GBUFFER_LAYERS,
      .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage =
          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT};

  if (vkCreateImage(
          logicalDevice, &gBufferImageCreateInfo, NULL, &renderImage->image)
      != VK_SUCCESS)
  {
    return false;
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(
      logicalDevice, renderImage->image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {
      .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size};

  if (!memory_type_find(memRequirements.memoryTypeBits, physicalDevice,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocInfo.memoryTypeIndex))
  {
    fprintf(stderr, "Could not find proper memory for the render image.\n");
    return false;
  }

  if (vkAllocateMemory(logicalDevice, &allocInfo, NULL, &renderImage->memory)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Could not allocate memory for the render image.\n");
    return false;
  }

  if (vkBindImageMemory(
          logicalDevice, renderImage->image, renderImage->memory, 0)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to bind memory to render image.\n");
    return false;
  }

  return true;
}

bool render_stack_create(RenderStack* renderStack, VkImage renderImage,
    VkExtent2D extents, VkFormat renderFormat, VkRenderPass renderPass,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  if (!render_stack_create_render_image(
          extents, physicalDevice, logicalDevice, &renderStack->gBufferImage))
  {
    return false;
  }

  VkImageViewCreateInfo gBufferImageViewCreateInfo = {
      .sType                         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                         = renderStack->gBufferImage.image,
      .viewType                      = VK_IMAGE_VIEW_TYPE_2D,
      .format                        = VK_FORMAT_R16G16B16A16_SFLOAT,
      .subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount   = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
  };

  for (int layer = 0; layer < _countof(renderStack->gBufferLayers); layer++)
  {
    gBufferImageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
    if (vkCreateImageView(logicalDevice, &gBufferImageViewCreateInfo, NULL,
            &renderStack->gBufferLayers[layer])
        != VK_SUCCESS)
    {
      return false;
    }
  }

  VkFramebufferCreateInfo gBufferFramebufferCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = renderPass,
      .attachmentCount = _countof(renderStack->gBufferLayers),
      .pAttachments    = renderStack->gBufferLayers,
      .width           = extents.width,
      .height          = extents.height,
      .layers          = 1,
  };

  if (vkCreateFramebuffer(logicalDevice, &gBufferFramebufferCreateInfo, NULL,
          &renderStack->gBuffer)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to create framebuffer from image view.\n");
    return false;
  }

  VkImageViewCreateInfo finalImageViewCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = renderImage,
      .format           = renderFormat,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseArrayLayer              = 0,
          .layerCount                  = 1,
          .baseMipLevel                = 0,
          .levelCount                  = 1}};

  if (vkCreateImageView(logicalDevice, &finalImageViewCreateInfo, NULL,
          &renderStack->renderImageView)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to get swapchain images.\n");
    return false;
  }

  VkFramebufferCreateInfo finalFramebufferCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = renderPass,
      .attachmentCount = 1,
      .pAttachments    = &renderStack->renderImageView,
      .width           = extents.width,
      .height          = extents.height,
      .layers          = 1,
  };

  if (vkCreateFramebuffer(logicalDevice, &finalFramebufferCreateInfo, NULL,
          &renderStack->renderBuffer)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to create framebuffer from image view.\n");
    return false;
  }
  return true;
}

void render_stack_destroy(RenderStack* renderStack, VkDevice logicalDevice)
{
  if (renderStack->gBuffer != VK_NULL_HANDLE)
  {
    vkDestroyFramebuffer(logicalDevice, renderStack->gBuffer, NULL);
  }
  for (int i = 0; i < _countof(renderStack->gBufferLayers); i++)
  {
    if (renderStack->gBufferLayers[i] != VK_NULL_HANDLE)
    {
      vkDestroyImageView(logicalDevice, renderStack->gBufferLayers[i], NULL);
    }
  }

  if (renderStack->gBufferImage.image != VK_NULL_HANDLE)
  {
    vkDestroyImage(logicalDevice, renderStack->gBufferImage.image, NULL);
  }
  if (renderStack->gBufferImage.memory != VK_NULL_HANDLE)
  {
    vkFreeMemory(logicalDevice, renderStack->gBufferImage.memory, NULL);
  }

  if (renderStack->renderImageView != VK_NULL_HANDLE)
  {
    vkDestroyImageView(logicalDevice, renderStack->renderImageView, NULL);
  }
  if (renderStack->renderBuffer != VK_NULL_HANDLE)
  {
    vkDestroyFramebuffer(logicalDevice, renderStack->renderBuffer, NULL);
  }
}
