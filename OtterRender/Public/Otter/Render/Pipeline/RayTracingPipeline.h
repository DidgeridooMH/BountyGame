#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/RayTracing/AccelerationStructure.h"
#include "Otter/Render/RenderStack.h"

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

void ray_tracing_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    RenderStack* renderStack, AccelerationStructure* accelerationStructure,
    RayTracingPipeline* pipeline);
