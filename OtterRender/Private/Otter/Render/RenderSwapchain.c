#include "Otter/Render/RenderSwapchain.h"

#include <vulkan/vulkan_core.h>

#include "Otter/Math/Clamp.h"
#include "Otter/Util/Log.h"

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
    LOG_ERROR("Unable to get surface capabilities.");
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
    LOG_ERROR("Unable to create swapchain");
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
    LOG_ERROR("OOM");
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

  return renderSwapchain;
}

void render_swapchain_destroy(
    RenderSwapchain* renderSwapchain, VkDevice logicalDevice)
{
  if (renderSwapchain->renderStacks != NULL)
  {
    for (size_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
    {
      LOG_DEBUG("Destroying render stack %llu", i);
      gbuffer_pass_destroy(
          &renderSwapchain->renderStacks[i].gbufferPass, logicalDevice);
      lighting_pass_destroy(
          &renderSwapchain->renderStacks[i].lightingPass, logicalDevice);
    }
    free(renderSwapchain->renderStacks);
  }

  vkDestroyRenderPass(logicalDevice, renderSwapchain->gbufferPass, NULL);
  vkDestroyRenderPass(logicalDevice, renderSwapchain->lightingPass, NULL);

  if (renderSwapchain->swapchain != VK_NULL_HANDLE)
  {
    LOG_DEBUG("Destroying swapchain");
    vkDestroySwapchainKHR(logicalDevice, renderSwapchain->swapchain, NULL);
  }

  free(renderSwapchain);
}

bool render_swapchain_create_render_stacks(RenderSwapchain* renderSwapchain,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  if (vkGetSwapchainImagesKHR(logicalDevice, renderSwapchain->swapchain,
          &renderSwapchain->numOfSwapchainImages, NULL)
          != VK_SUCCESS
      || renderSwapchain->numOfSwapchainImages == 0)
  {
    LOG_ERROR("Unable to get swapchain images.");
    return false;
  }

  VkImage* swapchainImages =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(VkImage));
  if (swapchainImages == NULL)
  {
    LOG_ERROR("OOM");
    return false;
  }

  if (vkGetSwapchainImagesKHR(logicalDevice, renderSwapchain->swapchain,
          &renderSwapchain->numOfSwapchainImages, swapchainImages)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to get swapchain images.");
    free(swapchainImages);
    return false;
  }

  renderSwapchain->renderStacks =
      calloc(renderSwapchain->numOfSwapchainImages, sizeof(RenderStack));
  if (renderSwapchain->renderStacks == NULL)
  {
    LOG_ERROR("OOM");
    free(swapchainImages);
    return false;
  }

  for (uint32_t i = 0; i < renderSwapchain->numOfSwapchainImages; i++)
  {
    if (!gbuffer_pass_create(&renderSwapchain->renderStacks[i].gbufferPass,
            renderSwapchain->gbufferPass, renderSwapchain->extents,
            physicalDevice, logicalDevice))
    {
      free(swapchainImages);
      return false;
    }

    if (!lighting_pass_create(&renderSwapchain->renderStacks[i].lightingPass,
            &renderSwapchain->renderStacks[i].gbufferPass,
            renderSwapchain->lightingPass, swapchainImages[i],
            renderSwapchain->extents, renderSwapchain->format.format,
            physicalDevice, logicalDevice))
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
    LOG_ERROR("Error: Could not wait for fence");
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
    LOG_ERROR("Error: Unable to retrieve image index");
    // TODO: Handle the error appropriately
    return false;
  }

  return true;
}

