#pragma once

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/Texture/RenderImage.h"
#include <vulkan/vulkan.h>

#define G_BUFFER_LAYERS 4

enum RenderStackLayers
{
  RSL_POSITION,
  RSL_NORMAL,
  RSL_COLOR,
  RSL_MATERIAL,
  RSL_LIGHTING,
  RSL_DEPTH,
  RSL_SHADOWMAP,
  NUM_OF_RENDER_STACK_LAYERS
};

typedef struct RenderStack
{
  RenderImage gBufferImage;
  VkImageView bufferAttachments[NUM_OF_RENDER_STACK_LAYERS];
  VkFramebuffer framebuffer;

  // TODO: Make this better by doing this async or optionally.
  RenderImage cpuShadowMap;
  GpuBuffer cpuShadowMapBuffer;
} RenderStack;

bool render_stack_create(RenderStack* renderStack, VkImage renderImage,
    VkImageView depthBuffer, VkExtent2D extents, VkFormat renderFormat,
    VkRenderPass renderPass, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice);

void render_stack_destroy(RenderStack* renderStack, VkDevice logicalDevice);
