#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Math/Transform.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/Pipeline/PbrPipeline.h"
#include "Otter/Render/RenderStack.h"
#include "Otter/Util/AutoArray.h"

typedef struct RenderFrame
{
  VkCommandBuffer commandBuffer;
  VkCommandPool secondaryCommandPool;
  AutoArray meshCommandBuffers;
  VkDescriptorPool descriptorPool;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inflightFence;

  AutoArray renderQueue;
  AutoArray perRenderBuffers;
} RenderFrame;

bool render_frame_create(RenderFrame* renderFrame, uint32_t graphicsQueueFamily,
    VkDevice logicalDevice, VkCommandPool commandPool);

void render_frame_destroy(RenderFrame* renderFrame, VkCommandPool commandPool,
    VkDevice logicalDevice);

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    Mesh* fullscreenQuad, Transform* camera, VkRenderPass renderPass,
    VkQueue queue, VkCommandPool commandPool, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice);

void render_frame_clear_buffers(
    RenderFrame* renderFrame, VkDevice logicalDevice);

