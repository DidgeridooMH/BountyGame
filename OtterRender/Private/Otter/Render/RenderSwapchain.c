#include "Otter/Render/RenderSwapchain.h"

static inline uint32_t clamp(uint32_t value, uint32_t minimum, uint32_t maximum)
{
  return max(min(value, maximum), minimum);
}

static bool render_swapchain_create_swapchain(RenderSwapchain* renderSwapchain,
    uint32_t requestedNumberOfFrames, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice, VkSurfaceKHR surface, uint32_t graphicsQueueFamily,
    uint32_t presentQueueFamily)
{
  VkSurfaceCapabilitiesKHR surfaceCaps;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
          physicalDevice, surface, &surfaceCaps)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to get surface capabilities.\n");
    return false;
  }

  // TODO: Look into different alpha flags.
  VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

  uint32_t numOfFrames = clamp(requestedNumberOfFrames,
      surfaceCaps.minImageCount, surfaceCaps.maxImageCount);

  VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface          = surface,
      .minImageCount    = numOfFrames,
      .imageFormat      = renderSwapchain->format,
      .imageColorSpace  = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
      .imageExtent      = renderSwapchain->extents,
      .imageArrayLayers = 1,
      .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha   = alpha,
      .clipped          = VK_TRUE};

  uint32_t queueFamilyIndices[] = {graphicsQueueFamily, presentQueueFamily};
  if (graphicsQueueFamily != presentQueueFamily)
  {
    swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchainCreateInfo.queueFamilyIndexCount =
        sizeof(queueFamilyIndices) / sizeof(queueFamilyIndices[0]);
    swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
  }

  if (vkCreateSwapchainKHR(logicalDevice, &swapchainCreateInfo, NULL,
          &renderSwapchain->swapchain)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to create swapchain\n");
    return false;
  }

  return true;
}

static bool render_swapchain_create_images(
    RenderSwapchain* renderSwapchain, VkDevice logicalDevice)
{
  if (vkGetSwapchainImagesKHR(logicalDevice, renderSwapchain->swapchain,
          &renderSwapchain->numOfSwapchainImages, NULL)
          != VK_SUCCESS
      || renderSwapchain->numOfSwapchainImages == 0)
  {
    fprintf(stderr, "Unable to get swapchain images.\n");
    return false;
  }

  renderSwapchain->swapchainImages =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(VkImage));
  if (renderSwapchain->swapchainImages == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  if (vkGetSwapchainImagesKHR(logicalDevice, renderSwapchain->swapchain,
          &renderSwapchain->numOfSwapchainImages,
          renderSwapchain->swapchainImages)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to get swapchain images.\n");
    free(renderSwapchain->swapchainImages);
    renderSwapchain->swapchainImages = NULL;
    return false;
  }

  renderSwapchain->imageViews =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(VkImageView));
  if (renderSwapchain->imageViews == NULL)
  {
    fprintf(stderr, "Unable to get swapchain images.\n");
    free(renderSwapchain->swapchainImages);
    renderSwapchain->swapchainImages = NULL;
    return false;
  }

  for (uint32_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
  {
    VkImageViewCreateInfo imageViewInfo = {
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = renderSwapchain->swapchainImages[i],
        .format           = renderSwapchain->format,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseArrayLayer              = 0,
            .layerCount                  = 1,
            .baseMipLevel                = 0,
            .levelCount                  = 1}};

    if (vkCreateImageView(logicalDevice, &imageViewInfo, NULL,
            &renderSwapchain->imageViews[i])
        != VK_SUCCESS)
    {
      fprintf(stderr, "Unable to get swapchain images.\n");
      free(renderSwapchain->swapchainImages);
      renderSwapchain->swapchainImages = NULL;
      free(renderSwapchain->imageViews);
      renderSwapchain->imageViews = NULL;
      return false;
    }
  }

  return true;
}

RenderSwapchain* render_swapchain_create(uint32_t requestedNumberOfFrames,
    VkExtent2D extents, VkFormat format, VkPresentModeKHR presentMode,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkSurfaceKHR surface, uint32_t graphicsQueueFamily,
    uint32_t presentQueueFamily)
{
  RenderSwapchain* renderSwapchain = calloc(1, sizeof(RenderSwapchain));
  if (renderSwapchain == NULL)
  {
    fprintf(stderr, "OOM\n");
    return NULL;
  }
  renderSwapchain->format  = format;
  renderSwapchain->extents = extents;

  if (!render_swapchain_create_swapchain(renderSwapchain,
          requestedNumberOfFrames, physicalDevice, logicalDevice, surface,
          graphicsQueueFamily, presentQueueFamily))
  {
    render_swapchain_destroy(renderSwapchain, logicalDevice);
    return NULL;
  }

  if (!render_swapchain_create_images(renderSwapchain, logicalDevice))
  {
    render_swapchain_destroy(renderSwapchain, logicalDevice);
    return NULL;
  }

  return renderSwapchain;
}

void render_swapchain_destroy(
    RenderSwapchain* renderSwapchain, VkDevice logicalDevice)
{
  // TODO: Should we wait for the fences to finish?

  if (renderSwapchain->framebuffers != NULL)
  {
    for (uint32_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
    {
      if (renderSwapchain->framebuffers[i] != VK_NULL_HANDLE)
      {
        vkDestroyFramebuffer(
            logicalDevice, renderSwapchain->framebuffers[i], NULL);
      }
    }
    free(renderSwapchain->framebuffers);
  }

  if (renderSwapchain->imageViews != NULL)
  {
    for (uint32_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
    {
      if (renderSwapchain->imageViews[i] != VK_NULL_HANDLE)
      {
        vkDestroyImageView(logicalDevice, renderSwapchain->imageViews[i], NULL);
      }
    }
    free(renderSwapchain->imageViews);
  }

  if (renderSwapchain->swapchain != VK_NULL_HANDLE)
  {
    if (renderSwapchain->swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(logicalDevice, renderSwapchain->swapchain, NULL);
    }
  }

  free(renderSwapchain);
}

bool render_swapchain_create_framebuffers(RenderSwapchain* renderSwapchain,
    VkDevice logicalDevice, VkRenderPass renderPass)
{
  renderSwapchain->framebuffers =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(VkFramebuffer));
  if (renderSwapchain->framebuffers == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  for (uint32_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
  {
    VkFramebufferCreateInfo framebufferCreateInfo = {
        .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass      = renderPass,
        .attachmentCount = 1,
        .pAttachments    = &renderSwapchain->imageViews[i],
        .width           = renderSwapchain->extents.width,
        .height          = renderSwapchain->extents.height,
        .layers          = 1,
    };

    if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, NULL,
            &renderSwapchain->framebuffers[i])
        != VK_SUCCESS)
    {
      fprintf(stderr, "Error: Unable to create framebuffer from image view.\n");
      return false;
    }
  }

  return true;
}

bool render_swapchain_get_next_image(RenderSwapchain* renderSwapchain,
    VkDevice logicalDevice, VkFence previousRenderFinished,
    VkSemaphore signalAfterAcquire, uint32_t* nextImage)
{
  if (vkWaitForFences(
          logicalDevice, 1, &previousRenderFinished, VK_TRUE, UINT64_MAX)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Could not wait for fence\n");
    // Handle the error appropriately
    return false;
  }

  VkResult result =
      vkAcquireNextImageKHR(logicalDevice, renderSwapchain->swapchain,
          UINT64_MAX, signalAfterAcquire, NULL, nextImage);
  if (result == VK_SUCCESS)
  {
    vkResetFences(logicalDevice, 1, &previousRenderFinished);
  }
  else if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
  {
    fprintf(stderr, "Error: Unable to retrieve image index\n");
    // TODO: Handle the error appropriately
    return false;
  }

  return true;
}
