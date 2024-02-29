#pragma once

#include "Otter/Render/RenderSwapchain.h"
#include "Otter/Render/export.h"
#include <vulkan/vulkan.h>

typedef struct RenderInstance
{
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice logicalDevice;
#ifdef _DEBUG
  VkDebugUtilsMessengerEXT debugMessenger;
#endif
  VkSurfaceKHR surface;
  VkRenderPass renderPass;
  RenderSwapchain* swapchain;
  uint32_t graphicsQueueFamily;
  uint32_t presentQueueFamily;
  uint32_t framesInFlight;
} RenderInstance;

OTTER_API RenderInstance* render_instance_create(HWND window);

OTTER_API void render_instance_destroy(RenderInstance* renderInstance);
