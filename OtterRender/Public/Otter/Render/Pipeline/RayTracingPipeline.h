#pragma once

#include <vulkan/vulkan.h>

typedef struct RayTracingPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts;
  VkPipelineLayout layout;
  VkPipeline pipeline;
} RayTracingPipeline;

bool ray_tracing_pipeline_create(const char* shaderDirectory,
    VkDevice logicalDevice, RayTracingPipeline* pipeline);
void ray_tracing_pipeline_destroy(
    RayTracingPipeline* pipeline, VkDevice logicalDevice);

