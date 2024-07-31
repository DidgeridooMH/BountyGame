#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/RenderPass/GBufferPass.h"
#include "Otter/Render/Texture/Image.h"

typedef struct LightingPass
{
  VkImageView finalImage;
  VkFramebuffer framebuffer;
  VkExtent2D imageSize;

  // TODO: Might move this to a shadow pass.
  Image shadowMap;
  VkImageView shadowMapView;
} LightingPass;

bool lighting_pass_create_render_pass(
    VkRenderPass* renderPass, VkFormat format, VkDevice logicalDevice);

bool lighting_pass_create(LightingPass* lightingPass, GBufferPass* gbufferPass,
    VkRenderPass renderPass, VkImage renderImage, VkExtent2D extents,
    VkFormat renderFormat, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice);

void lighting_pass_destroy(LightingPass* lightingPass, VkDevice logicalDevice);
