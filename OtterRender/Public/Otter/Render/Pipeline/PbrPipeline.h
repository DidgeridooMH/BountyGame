#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/RenderStack.h"

typedef struct PbrPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts;
  VkPipelineLayout layout;
  VkPipeline pipeline;
} PbrPipeline;

bool pbr_pipeline_create(
    VkDevice logicalDevice, VkRenderPass renderPass, PbrPipeline* material);
void pbr_pipeline_destroy(PbrPipeline* material, VkDevice logicalDevice);

void pbr_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    RenderStack* renderStack, PbrPipeline* pipeline);
