#pragma once

#include <vulkan/vulkan.h>

typedef struct RenderImage
{
  VkImage image;
  VkDeviceMemory memory;
} RenderImage;

enum GBufferLayer
{
  GBL_POSITION,
  GBL_NORMAL,
  GBL_COLOR,
  GBL_MATERIAL,
  NUM_OF_GBUFFER_LAYERS
};

typedef struct RenderStack
{
  RenderImage gBufferImage;
  VkImageView gBufferLayers[NUM_OF_GBUFFER_LAYERS];
  VkFramebuffer gBuffer;

  VkImageView renderImageView;
  VkFramebuffer renderBuffer;
} RenderStack;

bool render_stack_create(RenderStack* renderStack, VkImage renderImage,
    VkExtent2D extents, VkFormat renderFormat, VkRenderPass renderPass,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

void render_stack_destroy(RenderStack* renderStack, VkDevice logicalDevice);
