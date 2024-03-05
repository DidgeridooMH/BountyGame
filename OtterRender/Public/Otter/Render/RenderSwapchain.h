#pragma once

#include "Otter/Render/RenderStack.h"
#include <vulkan/vulkan.h>

typedef struct RenderSwapchain
{
  VkSwapchainKHR swapchain;
  VkSurfaceFormatKHR format;
  VkExtent2D extents;
  RenderStack* renderStacks;
  uint32_t numOfSwapchainImages;
} RenderSwapchain;

RenderSwapchain* render_swapchain_create(uint32_t requestedNumberOfFrames,
    VkExtent2D extents, VkSurfaceFormatKHR format, VkPresentModeKHR presentMode,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkSurfaceKHR surface, uint32_t graphicsQueueFamily,
    uint32_t presentQueueFamily);

void render_swapchain_destroy(
    RenderSwapchain* renderSwapchain, VkDevice logicalDevice);

bool render_swapchain_create_render_stacks(RenderSwapchain* renderSwapchain,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkRenderPass renderPass);

bool render_swapchain_get_next_image(RenderSwapchain* swapchain,
    VkDevice logicalDevice, VkFence previousRenderFinished,
    VkSemaphore signalAfterAcquire, uint32_t* nextImage);
