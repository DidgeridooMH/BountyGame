#pragma once

#include "Otter/Render/export.h"
#include <vulkan/vulkan.h>

typedef struct GBufferPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts;
  VkPipelineLayout layout;
  VkPipeline pipeline;
} GBufferPipeline;

bool g_buffer_pipeline_create(
    VkDevice logicalDevice, VkRenderPass renderPass, GBufferPipeline* pipeline);

void g_buffer_pipeline_destroy(
    GBufferPipeline* pipeline, VkDevice logicalDevice);
