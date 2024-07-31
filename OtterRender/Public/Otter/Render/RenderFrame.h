#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Math/Transform.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/GBufferPipeline.h"
#include "Otter/Render/Pipeline/PbrPipeline.h"
#include "Otter/Render/Pipeline/RayTracingPipeline.h"
#include "Otter/Render/RayTracing/AccelerationStructure.h"
#include "Otter/Render/RayTracing/ShaderBindingTable.h"
#include "Otter/Render/RenderStack.h"
#include "Otter/Util/AutoArray.h"

typedef struct RenderFrame
{
  VkCommandBuffer gBufferCommandBuffer;
  VkCommandBuffer lightingCommandBuffer;
  VkCommandBuffer shadowCommandBuffer;

  AutoArray secondaryCommandPools;
  AutoArray meshCommandBufferLists;
  AutoArray descriptorPools;
  VkSemaphore imageAvailableSemaphore;
  VkSemaphore gBufferFinishedSemaphore;
  VkSemaphore shadowFinishedSemaphore;
  VkSemaphore renderFinishedSemaphore;
  VkFence inflightFence;

  GpuBuffer vpBuffer;
  GpuBuffer lightBuffer;

  AutoArray recordTasks;
  AutoArray recordCommands;

  AutoArray renderQueue;
  AutoArray perRenderBuffers;

  // TODO: this should only be created when raytracing is enabled.
  bool accelerationStructureInitialized;
  AccelerationStructure accelerationStructure;

  // TODO: Do initialization better.
  bool shadowMapInitialized;
} RenderFrame;

bool render_frame_create(RenderFrame* renderFrame, uint32_t graphicsQueueFamily,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandPool commandPool);

void render_frame_destroy(RenderFrame* renderFrame, VkCommandPool commandPool,
    VkDevice logicalDevice);

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    RayTracingPipeline* rayTracingPipeline, ShaderBindingTable* sbt,
    Mesh* fullscreenQuad, Transform* camera, VkRenderPass gbufferPass,
    VkRenderPass lightingPass, VkQueue queue, VkCommandPool commandPool,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice);

void render_frame_clear_buffers(
    RenderFrame* renderFrame, VkDevice logicalDevice);
