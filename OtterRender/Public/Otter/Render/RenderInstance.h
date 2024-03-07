#pragma once

#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/Pipeline/PbrPipeline.h"
#include "Otter/Render/RenderFrame.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/RenderSwapchain.h"
#include "Otter/Render/export.h"
#include "Otter/Util/LinkedList.h"
#include <vulkan/vulkan.h>

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

  VkCommandPool commandPool;

  uint32_t currentFrame;

  uint32_t graphicsQueueFamily;
  uint32_t presentQueueFamily;

  RenderFrame* frames;
  uint32_t framesInFlight;

  RenderSettings settings;
  RenderCapabilities capabilities;

  GBufferPipeline gBufferPipeline;
  PbrPipeline pbrPipeline;
  // TODO: Rewrite mesh to not have to be a pointer.
  Mesh* fullscreenQuad;

  RenderCommand command;
  Vec3 cameraPosition;
} RenderInstance;

OTTERRENDER_API RenderInstance* render_instance_create(HWND window);

OTTERRENDER_API void render_instance_destroy(RenderInstance* renderInstance);

OTTERRENDER_API void render_instance_draw(RenderInstance* renderInstance);
