#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Texture/Image.h"

typedef enum GBufferPassLayers
{
  GBL_POSITION,
  GBL_NORMAL,
  GBL_COLOR,
  GBL_MATERIAL,
  NUM_OF_GBUFFER_PASS_LAYERS
} GBufferPassLayers;

typedef struct GBufferPass
{
  // TODO: Get rid of position and do depth reconstruction.
  Image bufferImages[NUM_OF_GBUFFER_PASS_LAYERS];
  Image depthBuffer;
  VkImageView bufferAttachments[NUM_OF_GBUFFER_PASS_LAYERS + 1];
  VkFramebuffer framebuffer;
} GBufferPass;

bool gbuffer_pass_create_render_pass(
    VkRenderPass* renderPass, VkDevice logicalDevice);

bool gbuffer_pass_create(GBufferPass* gbufferPass, VkRenderPass renderPass,
    VkExtent2D extents, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice);

void gbuffer_pass_destroy(GBufferPass* gbufferPass, VkDevice logicalDevice);
