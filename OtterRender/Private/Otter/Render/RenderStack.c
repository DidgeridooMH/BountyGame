#include "Otter/Render/RenderStack.h"

#include "Otter/Util/Log.h"

bool render_stack_create(RenderStack* renderStack, VkImage renderImage,
    VkExtent2D extents, VkFormat renderFormat, VkRenderPass renderPass,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  // TODO: Check if this is correct for HDR.
  if (!image_create(extents, G_BUFFER_LAYERS, VK_FORMAT_R16G16B16A16_SFLOAT,
          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
          false, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice,
          logicalDevice, &renderStack->gBufferImage))
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

  for (int layer = 0; layer < G_BUFFER_LAYERS; layer++)
  {
    gBufferImageViewCreateInfo.subresourceRange.baseArrayLayer = layer;
    if (vkCreateImageView(logicalDevice, &gBufferImageViewCreateInfo, NULL,
            &renderStack->bufferAttachments[layer])
        != VK_SUCCESS)
    {
      return false;
    }
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
          &renderStack->bufferAttachments[RSL_LIGHTING])
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to get swapchain images.");
    return false;
  }

  if (!image_create(extents, 1, VK_FORMAT_D32_SFLOAT,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, false,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice,
          &renderStack->depthBuffer))
  {
    return false;
  }

  VkImageViewCreateInfo depthBufferImageViewCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = renderStack->depthBuffer.image,
      .format           = VK_FORMAT_D32_SFLOAT,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseArrayLayer              = 0,
          .layerCount                  = 1,
          .baseMipLevel                = 0,
          .levelCount                  = 1}};

  if (vkCreateImageView(logicalDevice, &depthBufferImageViewCreateInfo, NULL,
          &renderStack->bufferAttachments[RSL_DEPTH])
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to get depth buffer image view.");
    return false;
  }

  VkFramebufferCreateInfo framebufferCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = renderPass,
      .attachmentCount = _countof(renderStack->bufferAttachments),
      .pAttachments    = renderStack->bufferAttachments,
      .width           = extents.width,
      .height          = extents.height,
      .layers          = 1,
  };
  if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, NULL,
          &renderStack->framebuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to create framebuffer from image view.");
    return false;
  }

  return true;
}

void render_stack_destroy(RenderStack* renderStack, VkDevice logicalDevice)
{
  if (renderStack->framebuffer != VK_NULL_HANDLE)
  {
    LOG_DEBUG("Destroying framebuffer %llx", renderStack->framebuffer);
    vkDestroyFramebuffer(logicalDevice, renderStack->framebuffer, NULL);
  }
  for (int i = 0; i < _countof(renderStack->bufferAttachments); i++)
  {
    if (renderStack->bufferAttachments[i] != VK_NULL_HANDLE)
    {
      LOG_DEBUG(
          "Destroying attachment %llx", renderStack->bufferAttachments[i]);
      vkDestroyImageView(
          logicalDevice, renderStack->bufferAttachments[i], NULL);
    }
  }

  image_destroy(&renderStack->gBufferImage, logicalDevice);
  image_destroy(&renderStack->depthBuffer, logicalDevice);
}
