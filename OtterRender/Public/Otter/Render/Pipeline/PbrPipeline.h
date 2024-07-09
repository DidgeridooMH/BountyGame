#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Math/Vec.h"
#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/RenderStack.h"

typedef struct LightingData
{
  Vec3 cameraPositionWorldSpace;
} LightingData;

typedef struct PbrPipeline
{
  VkDescriptorSetLayout descriptorSetLayouts;
  VkPipelineLayout layout;
  VkPipeline pipeline;
} PbrPipeline;

bool pbr_pipeline_create(const char* shaderDirectory, VkDevice logicalDevice,
    VkRenderPass renderPass, PbrPipeline* material);
void pbr_pipeline_destroy(PbrPipeline* material, VkDevice logicalDevice);

void pbr_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    RenderStack* renderStack, GpuBuffer* pbrData, PbrPipeline* pipeline);
