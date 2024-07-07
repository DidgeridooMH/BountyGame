#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Memory/GpuBuffer.h"

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

void g_buffer_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    GpuBuffer* mvpBuffer, GBufferPipeline* pipeline);
