#pragma once

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/Pipeline/PbrPipeline.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/RenderStack.h"
#include "Otter/Util/AutoArray.h"
#include <vulkan/vulkan.h>

typedef struct RenderFrame
{
  VkCommandBuffer commandBuffer;
  VkDescriptorPool descriptorPool;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inflightFence;

  AutoArray renderQueue;
  AutoArray perRenderBuffers;
} RenderFrame;

bool render_frame_create(RenderFrame* renderFrame, VkDevice logicalDevice,
    VkCommandPool commandPool);

void render_frame_destroy(RenderFrame* renderFrame, VkCommandPool commandPool,
    VkDevice logicalDevice);

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    Mesh* fullscreenQuad, Vec3* camera, VkExtent2D extents,
    VkRenderPass renderPass, VkQueue queue, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice);

void render_frame_clear_buffers(
    RenderFrame* renderFrame, VkDevice logicalDevice);
