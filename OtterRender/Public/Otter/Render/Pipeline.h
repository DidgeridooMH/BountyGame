#pragma once

#include "Otter/Render/export.h"
#include <vulkan/vulkan.h>

typedef struct Pipeline
{
  VkPipelineLayout layout;
  VkPipeline pipeline;
} Pipeline;

OTTERRENDER_API Pipeline* pipeline_create(
    const char* name, VkDevice logicalDevice, VkRenderPass renderPass);

OTTERRENDER_API void pipeline_destroy(
    Pipeline* pipeline, VkDevice logicalDevice);
