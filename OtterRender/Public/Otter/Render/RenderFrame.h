#pragma once

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/RenderStack.h"
#include <vulkan/vulkan.h>

typedef struct RenderFrame
{
  VkCommandBuffer commandBuffer;
  VkDescriptorPool descriptorPool;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inflightFence;

  GpuBuffer* perRenderBuffers;
  uint32_t perRenderBuffersCount;
  uint32_t perRenderBuffersCapacity;
} RenderFrame;

bool render_frame_create(RenderFrame* renderFrame, VkDevice logicalDevice,
    VkCommandPool commandPool);

void render_frame_destroy(RenderFrame* renderFrame, VkCommandPool commandPool,
    VkDevice logicalDevice);

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    RenderCommand* command, GBufferPipeline* gBufferPipeline,
    VkExtent2D extents, VkRenderPass renderPass, VkQueue queue,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice);
