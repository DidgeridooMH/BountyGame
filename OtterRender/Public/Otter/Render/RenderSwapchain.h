#pragma once

#include <vulkan/vulkan.h>

typedef struct RenderSwapchain
{
  VkSwapchainKHR swapchain;
  VkFormat format;
  VkExtent2D extents;
  VkImage* swapchainImages;
  VkImageView* imageViews;
  VkFramebuffer* framebuffers;
  uint32_t numOfSwapchainImages;
} RenderSwapchain;

RenderSwapchain* render_swapchain_create(uint32_t requestedNumberOfFrames,
    VkExtent2D extents, VkFormat format, VkPresentModeKHR presentMode,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkSurfaceKHR surface, uint32_t graphicsQueueFamily,
    uint32_t presentQueueFamily);

void render_swapchain_destroy(
    RenderSwapchain* renderSwapchain, VkDevice logicalDevice);

bool render_swapchain_create_framebuffers(RenderSwapchain* renderSwapchain,
    VkDevice logicalDevice, VkRenderPass renderPass);
