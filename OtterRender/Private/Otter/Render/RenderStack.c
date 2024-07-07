#include "Otter/Render/RenderStack.h"

bool render_stack_create(RenderStack* renderStack, VkImage renderImage,
    VkImageView depthBuffer, VkExtent2D extents, VkFormat renderFormat,
    VkRenderPass renderPass, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  if (!render_image_create(extents, G_BUFFER_LAYERS,
          VK_FORMAT_R16G16B16A16_SFLOAT,
          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice,
          &renderStack->gBufferImage))
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
    fprintf(stderr, "Unable to get swapchain images.\n");
    return false;
  }

  renderStack->bufferAttachments[RSL_DEPTH] = depthBuffer;

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
    fprintf(stderr, "Error: Unable to create framebuffer from image view.\n");
    return false;
  }

  return true;
}

void render_stack_destroy(RenderStack* renderStack, VkDevice logicalDevice)
{
  if (renderStack->framebuffer != VK_NULL_HANDLE)
  {
    vkDestroyFramebuffer(logicalDevice, renderStack->framebuffer, NULL);
  }
  for (int i = 0; i < _countof(renderStack->bufferAttachments); i++)
  {
    if (renderStack->bufferAttachments[i] != VK_NULL_HANDLE)
    {
      vkDestroyImageView(
          logicalDevice, renderStack->bufferAttachments[i], NULL);
    }
  }

  render_image_destroy(&renderStack->gBufferImage, logicalDevice);
}
