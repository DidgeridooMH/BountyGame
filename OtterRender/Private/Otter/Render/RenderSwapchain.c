#include "Otter/Render/RenderSwapchain.h"

#include "Otter/Render/Memory/MemoryType.h"
#include "Otter/Math/Clamp.h"

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
      .imageFormat      = renderSwapchain->format.format,
      .imageColorSpace  = renderSwapchain->format.colorSpace,
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

RenderSwapchain* render_swapchain_create(uint32_t requestedNumberOfFrames,
    VkExtent2D extents, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode,
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

  // TODO: Generalize image creation.
  VkImageCreateInfo depthBufferCreateInfo = {
      .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .extent = {.width = extents.width, .height = extents.height, .depth = 1},
      .mipLevels     = 1,
      .samples       = VK_SAMPLE_COUNT_1_BIT,
      .arrayLayers   = 1,
      .format        = VK_FORMAT_D32_SFLOAT,
      .tiling        = VK_IMAGE_TILING_OPTIMAL,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .usage         = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT};
  if (vkCreateImage(logicalDevice, &depthBufferCreateInfo, NULL,
          &renderSwapchain->depthBuffer)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to create depth buffer image.\n");
    return NULL;
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(
      logicalDevice, renderSwapchain->depthBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {
      .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size};

  if (!memory_type_find(memRequirements.memoryTypeBits, physicalDevice,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &allocInfo.memoryTypeIndex))
  {
    fprintf(
        stderr, "Could not find proper memory for the depth buffer image.\n");
    return NULL;
  }

  if (vkAllocateMemory(
          logicalDevice, &allocInfo, NULL, &renderSwapchain->depthBufferMemory)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Could not allocate memory for the depth buffer image.\n");
    return NULL;
  }

  if (vkBindImageMemory(logicalDevice, renderSwapchain->depthBuffer,
          renderSwapchain->depthBufferMemory, 0)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to bind memory to depth buffer image.\n");
    return NULL;
  }

  VkImageViewCreateInfo depthBufferImageViewCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = renderSwapchain->depthBuffer,
      .format           = VK_FORMAT_D32_SFLOAT,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseArrayLayer              = 0,
          .layerCount                  = 1,
          .baseMipLevel                = 0,
          .levelCount                  = 1}};

  if (vkCreateImageView(logicalDevice, &depthBufferImageViewCreateInfo, NULL,
          &renderSwapchain->depthBufferView)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to get depth buffer image view.\n");
    return NULL;
  }

  return renderSwapchain;
}

void render_swapchain_destroy(
    RenderSwapchain* renderSwapchain, VkDevice logicalDevice)
{
  // TODO: Should we wait for the fences to finish?

  if (renderSwapchain->renderStacks != NULL)
  {
    render_stack_destroy(renderSwapchain->renderStacks, logicalDevice);
    free(renderSwapchain->renderStacks);
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

bool render_swapchain_create_render_stacks(RenderSwapchain* renderSwapchain,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkRenderPass renderPass)
{
  if (vkGetSwapchainImagesKHR(logicalDevice, renderSwapchain->swapchain,
          &renderSwapchain->numOfSwapchainImages, NULL)
          != VK_SUCCESS
      || renderSwapchain->numOfSwapchainImages == 0)
  {
    fprintf(stderr, "Unable to get swapchain images.\n");
    return false;
  }

  VkImage* swapchainImages =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(VkImage));
  if (swapchainImages == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  if (vkGetSwapchainImagesKHR(logicalDevice, renderSwapchain->swapchain,
          &renderSwapchain->numOfSwapchainImages, swapchainImages)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to get swapchain images.\n");
    free(swapchainImages);
    return false;
  }

  renderSwapchain->renderStacks =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(RenderStack));
  if (renderSwapchain->renderStacks == NULL)
  {
    fprintf(stderr, "OOM\n");
    free(swapchainImages);
    return false;
  }

  for (uint32_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
  {
    if (!render_stack_create(&renderSwapchain->renderStacks[i],
            swapchainImages[i], renderSwapchain->depthBufferView,
            renderSwapchain->extents, renderSwapchain->format.format,
            renderPass, physicalDevice, logicalDevice))
    {
      free(swapchainImages);
      return false;
    }
  }

  free(swapchainImages);

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
