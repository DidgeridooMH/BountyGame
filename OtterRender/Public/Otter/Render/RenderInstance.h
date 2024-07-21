#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Math/Transform.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/Pipeline/PbrPipeline.h"
#include "Otter/Render/RenderFrame.h"
#include "Otter/Render/RenderSwapchain.h"
#include "Otter/Render/Uniform/Material.h"
#include "Otter/Render/export.h"

enum DescriptorPoolId
{
  DPI_UNIFORM_BUFFERS,
  DPI_NUM_OF_POOLS
};

typedef struct RenderCapabilities
{
  bool hdr;
  bool debugUtils;
} RenderCapabilities;

typedef struct RenderSettings
{
  bool hdr;
  bool vsync;
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
  Mesh fullscreenQuad;

  Transform cameraTransform;
} RenderInstance;

OTTERRENDER_API RenderInstance* render_instance_create(
    HWND window, const char* shaderDirectory);

OTTERRENDER_API void render_instance_wait_for_idle(
    RenderInstance* renderInstance);

OTTERRENDER_API void render_instance_destroy(RenderInstance* renderInstance);

OTTERRENDER_API void render_instance_draw(RenderInstance* renderInstance);

OTTERRENDER_API void render_instance_queue_mesh_draw(Mesh* mesh,
    const Material* material, const Mat4 transform,
    RenderInstance* renderInstance);
