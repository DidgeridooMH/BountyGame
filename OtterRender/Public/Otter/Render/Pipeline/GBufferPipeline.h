#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Memory/GpuBuffer.h"

typedef struct GBufferPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts;
  VkPipelineLayout layout;
  VkPipeline pipeline;
} GBufferPipeline;

bool g_buffer_pipeline_create(const char* shaderDirectory,
    VkDevice logicalDevice, VkRenderPass renderPass, GBufferPipeline* pipeline);

void g_buffer_pipeline_destroy(
    GBufferPipeline* pipeline, VkDevice logicalDevice);

void g_buffer_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    GpuBuffer* vpBuffer, GBufferPipeline* pipeline);
