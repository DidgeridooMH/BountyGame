#pragma once

#include "Otter/Render/export.h"
#include <vulkan/vulkan.h>

typedef struct GBufferPipeline
{
  VkPipelineLayout layout;
  VkPipeline pipeline;
} GBufferPipeline;

OTTERRENDER_API bool g_buffer_pipeline_create(
    VkDevice logicalDevice, VkRenderPass renderPass, GBufferPipeline* pipeline);

OTTERRENDER_API void g_buffer_pipeline_destroy(
    GBufferPipeline* pipeline, VkDevice logicalDevice);
