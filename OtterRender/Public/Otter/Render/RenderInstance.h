#pragma once

#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/RenderSwapchain.h"
#include "Otter/Render/export.h"
#include <vulkan/vulkan.h>

#define DESCRIPTOR_POOL_SIZE 128
#define DESCRIPTOR_SET_LIMIT 512

enum DescriptorPoolId
{
  DPI_UNIFORM_BUFFERS,
  DPI_NUM_OF_POOLS
};

// TODO: Create separate struct for per frame data.

typedef struct RenderCapabilities
{
  bool hdr;
  bool debugUtils;
} RenderCapabilities;

typedef struct RenderSettings
{
  bool hdr;
} RenderSettings;

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

  VkDescriptorPool* descriptorPools;

  VkCommandPool commandPool;
  VkCommandBuffer* commandBuffers;

  VkSemaphore* imageAvailableSemaphores;
  VkSemaphore* renderFinishedSemaphores;
  VkFence* inflightFences;

  uint32_t currentFrame;

  uint32_t graphicsQueueFamily;
  uint32_t presentQueueFamily;
  uint32_t framesInFlight;

  RenderSettings settings;
  RenderCapabilities capabilities;

  GBufferPipeline gBufferPipeline;

  RenderCommand command;
} RenderInstance;

OTTERRENDER_API RenderInstance* render_instance_create(HWND window);

OTTERRENDER_API void render_instance_destroy(RenderInstance* renderInstance);

OTTERRENDER_API void render_instance_draw(RenderInstance* renderInstance);
