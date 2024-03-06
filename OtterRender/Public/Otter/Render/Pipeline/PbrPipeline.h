#pragma once

#include <vulkan/vulkan.h>

typedef struct PbrPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts;
  VkPipelineLayout layout;
  VkPipeline pipeline;
} PbrPipeline;

bool pbr_pipeline_create(
    VkDevice logicalDevice, VkRenderPass renderPass, PbrPipeline* material);
void pbr_pipeline_destroy(PbrPipeline* material, VkDevice logicalDevice);
