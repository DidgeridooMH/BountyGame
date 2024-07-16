#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/Uniform/Material.h"

typedef enum DescriptorSetIndex
{
  DSI_VP       = 0,
  DSI_MATERIAL = 1
} DescriptorSetIndex;

typedef struct GBufferPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts[2];
  VkPipelineLayout layout;
  VkPipeline pipeline;
} GBufferPipeline;

bool g_buffer_pipeline_create(const char* shaderDirectory,
    VkDevice logicalDevice, VkRenderPass renderPass, GBufferPipeline* pipeline);

void g_buffer_pipeline_destroy(
    GBufferPipeline* pipeline, VkDevice logicalDevice);

void g_buffer_pipeline_write_vp(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    GpuBuffer* vpBuffer, GBufferPipeline* pipeline);

void g_buffer_pipeline_write_material(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice, Material* material,
    GBufferPipeline* pipeline);
