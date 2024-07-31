#include "Otter/Render/RenderFrame.h"

#include <vulkan/vulkan_core.h>

#include "Otter/Async/Scheduler.h"
#include "Otter/Math/Projection.h"
#include "Otter/Render/RayTracing/RayTracingFunctions.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/Uniform/ViewProjection.h"
#include "Otter/Util/AutoArray.h"
#include "Otter/Util/Profiler.h"

#define DESCRIPTOR_POOL_SIZE 64
#define DESCRIPTOR_SET_LIMIT 8 * 1024

typedef struct RecordGBufferCommandsParams
{
  RenderFrame* renderFrame;
  size_t meshCommandIndex;
  size_t meshCommandCount;
  VkRenderPass renderPass;
  GBufferPipeline* gBufferPipeline;
  VkFramebuffer framebuffer;
  VkDevice logicalDevice;
  VkPhysicalDevice physicalDevice;
  VkExtent2D imageSize;
} RecordGBufferCommandsParams;

bool render_frame_create(RenderFrame* renderFrame, uint32_t graphicsQueueFamily,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandPool commandPool)
{
  memset(renderFrame, 0, sizeof(RenderFrame));

  VkCommandPoolCreateInfo poolCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .queueFamilyIndex = graphicsQueueFamily};

  const int numberOfThreads = task_scheduler_get_number_of_threads();

  auto_array_create(&renderFrame->secondaryCommandPools, sizeof(VkCommandPool));
  auto_array_allocate_many(
      &renderFrame->secondaryCommandPools, numberOfThreads);

  auto_array_create(&renderFrame->meshCommandBufferLists, sizeof(AutoArray));
  auto_array_allocate_many(
      &renderFrame->meshCommandBufferLists, numberOfThreads);

  for (int i = 0; i < numberOfThreads; i++)
  {
    AutoArray* meshCommandBuffers =
        auto_array_get(&renderFrame->meshCommandBufferLists, i);
    auto_array_create(meshCommandBuffers, sizeof(VkCommandBuffer));

    VkCommandPool* pool =
        auto_array_get(&renderFrame->secondaryCommandPools, i);
    if (vkCreateCommandPool(logicalDevice, &poolCreateInfo, NULL, pool)
        != VK_SUCCESS)
    {
      LOG_ERROR("Error: Failed to create secondary command pool");
      return false;
    }
  }

  VkCommandBufferAllocateInfo allocInfo = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = commandPool,
      .commandBufferCount = 1,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY};

  if (vkAllocateCommandBuffers(
          logicalDevice, &allocInfo, &renderFrame->gBufferCommandBuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Failed to allocate command buffers");
    return false;
  }

  if (vkAllocateCommandBuffers(
          logicalDevice, &allocInfo, &renderFrame->lightingCommandBuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Failed to allocate command buffers");
    return false;
  }

  if (vkAllocateCommandBuffers(
          logicalDevice, &allocInfo, &renderFrame->shadowCommandBuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Failed to allocate command buffers");
    return false;
  }

  VkDescriptorPoolSize poolSizes[] = {
      {.type               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = DESCRIPTOR_POOL_SIZE}};

  auto_array_create(&renderFrame->descriptorPools, sizeof(VkDescriptorPool));
  auto_array_allocate_many(&renderFrame->descriptorPools, numberOfThreads);

  VkDescriptorPoolCreateInfo createInfo = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets       = DESCRIPTOR_SET_LIMIT,
      .poolSizeCount = _countof(poolSizes),
      .pPoolSizes    = poolSizes};

  for (int i = 0; i < numberOfThreads; i++)
  {
    VkDescriptorPool* descriptorPool =
        auto_array_get(&renderFrame->descriptorPools, i);
    if (vkCreateDescriptorPool(logicalDevice, &createInfo, NULL, descriptorPool)
        != VK_SUCCESS)
    {
      LOG_ERROR("Could not create descriptor pools");
      return false;
    }
  }

  VkSemaphoreCreateInfo semaphoreInfo = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

  VkFenceCreateInfo fenceInfo = {.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags                            = VK_FENCE_CREATE_SIGNALED_BIT};

  if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL,
          &renderFrame->imageAvailableSemaphore)
          != VK_SUCCESS
      || vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL,
             &renderFrame->gBufferFinishedSemaphore)
             != VK_SUCCESS
      || vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL,
             &renderFrame->shadowFinishedSemaphore)
             != VK_SUCCESS
      || vkCreateSemaphore(logicalDevice, &semaphoreInfo, NULL,
             &renderFrame->renderFinishedSemaphore)
             != VK_SUCCESS
      || vkCreateFence(
             logicalDevice, &fenceInfo, NULL, &renderFrame->inflightFence)
             != VK_SUCCESS)
  {
    LOG_ERROR("Error: Failed to create synchronization objects");
    return false;
  }

  if (!gpu_buffer_allocate(&renderFrame->vpBuffer, sizeof(ViewProjection),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    LOG_ERROR("Error: Failed to allocate VP buffer");
    return false;
  }

  if (!gpu_buffer_allocate(&renderFrame->lightBuffer, sizeof(LightingData),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    LOG_ERROR("Warning: Could not allocate temporary gpu buffer...but I "
              "also don't know what to do about that.");
    return false;
  }

  auto_array_create(&renderFrame->recordTasks, sizeof(HANDLE));
  auto_array_create(
      &renderFrame->recordCommands, sizeof(RecordGBufferCommandsParams));

  auto_array_create(&renderFrame->renderQueue, sizeof(RenderCommand));
  auto_array_create(&renderFrame->perRenderBuffers, sizeof(GpuBuffer));

  acceleration_structure_create(&renderFrame->accelerationStructure);

  renderFrame->accelerationStructureInitialized = false;
  renderFrame->shadowMapInitialized             = false;

  return true;
}

void render_frame_destroy(
    RenderFrame* renderFrame, VkCommandPool commandPool, VkDevice logicalDevice)
{
  if (renderFrame->inflightFence != VK_NULL_HANDLE)
  {
    vkWaitForFences(
        logicalDevice, 1, &renderFrame->inflightFence, true, UINT64_MAX);
  }

  for (uint32_t i = 0; i < renderFrame->perRenderBuffers.size; i++)
  {
    gpu_buffer_free(
        auto_array_get(&renderFrame->perRenderBuffers, i), logicalDevice);
  }

  for (int i = 0; i < renderFrame->meshCommandBufferLists.size; i++)
  {
    AutoArray* meshCommandBuffers =
        auto_array_get(&renderFrame->meshCommandBufferLists, i);
    VkCommandPool* commandPool =
        auto_array_get(&renderFrame->secondaryCommandPools, i);
    if (meshCommandBuffers->size > 0)
    {
      vkFreeCommandBuffers(logicalDevice, *commandPool,
          meshCommandBuffers->size, meshCommandBuffers->buffer);
    }
    auto_array_destroy(meshCommandBuffers);

    vkDestroyCommandPool(logicalDevice, *commandPool, NULL);
  }
  auto_array_destroy(&renderFrame->meshCommandBufferLists);
  auto_array_destroy(&renderFrame->secondaryCommandPools);

  gpu_buffer_free(&renderFrame->vpBuffer, logicalDevice);
  gpu_buffer_free(&renderFrame->lightBuffer, logicalDevice);

  auto_array_destroy(&renderFrame->recordTasks);
  auto_array_destroy(&renderFrame->recordCommands);

  auto_array_destroy(&renderFrame->perRenderBuffers);
  auto_array_destroy(&renderFrame->renderQueue);

  acceleration_structure_destroy(
      &renderFrame->accelerationStructure, logicalDevice);

  vkDestroySemaphore(logicalDevice, renderFrame->imageAvailableSemaphore, NULL);
  vkDestroySemaphore(
      logicalDevice, renderFrame->gBufferFinishedSemaphore, NULL);
  vkDestroySemaphore(logicalDevice, renderFrame->shadowFinishedSemaphore, NULL);
  vkDestroySemaphore(logicalDevice, renderFrame->renderFinishedSemaphore, NULL);
  vkDestroyFence(logicalDevice, renderFrame->inflightFence, NULL);

  if (renderFrame->gBufferCommandBuffer)
  {
    vkFreeCommandBuffers(
        logicalDevice, commandPool, 1, &renderFrame->gBufferCommandBuffer);
  }

  if (renderFrame->lightingCommandBuffer)
  {
    vkFreeCommandBuffers(
        logicalDevice, commandPool, 1, &renderFrame->lightingCommandBuffer);
  }

  if (renderFrame->shadowCommandBuffer)
  {
    vkFreeCommandBuffers(
        logicalDevice, commandPool, 1, &renderFrame->shadowCommandBuffer);
  }

  for (int i = 0; i < renderFrame->descriptorPools.size; i++)
  {
    VkDescriptorPool* descriptorPool =
        auto_array_get(&renderFrame->descriptorPools, i);
    if (*descriptorPool != VK_NULL_HANDLE)
    {
      vkDestroyDescriptorPool(logicalDevice, *descriptorPool, NULL);
    }
  }
}

static void render_frame_draw_mesh(RenderCommand* meshCommand,
    VkCommandBuffer commandBuffer, GBufferPipeline* gBufferPipeline,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  vkCmdPushConstants(commandBuffer, gBufferPipeline->layout,
      VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &meshCommand->transform);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      commandBuffer, 0, 1, &meshCommand->mesh->vertices.buffer, &offset);
  vkCmdBindIndexBuffer(commandBuffer, meshCommand->mesh->indices.buffer, 0,
      VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(commandBuffer,
      (uint32_t) meshCommand->mesh->indices.size / sizeof(uint16_t), 1, 0, 0,
      0);
}

static void render_frame_start_pass(VkCommandBuffer commandBuffer,
    VkRenderPass renderPass, VkFramebuffer framebuffer, VkExtent2D extents,
    const VkClearValue clearValues[], size_t clearValuesCount,
    VkSubpassContents subpassContents, VkDevice logicalDevice)
{
  vkResetCommandBuffer(commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags            = 0,
      .pInheritanceInfo = NULL};

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to begin command buffer");
    // TODO: Handle error
    return;
  }

  VkRenderPassBeginInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass      = renderPass,
      .framebuffer     = framebuffer,
      .renderArea      = {{0, 0}, extents},
      .clearValueCount = clearValuesCount,
      .pClearValues    = clearValues};

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, subpassContents);
}

static void render_frame_end_pass(VkCommandBuffer commandBuffer)
{
  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to end recording of command buffer");
    // Handle error
    return;
  }
}

static void render_frame_record_g_buffer_commands(
    RecordGBufferCommandsParams* params, int threadId)
{
  VkCommandPool* commandPool =
      auto_array_get(&params->renderFrame->secondaryCommandPools, threadId);

  VkCommandBufferAllocateInfo allocInfo = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = *commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
      .commandBufferCount = 1};

  AutoArray* meshCommandBuffer =
      auto_array_get(&params->renderFrame->meshCommandBufferLists, threadId);
  VkCommandBuffer* commandBuffer = auto_array_allocate(meshCommandBuffer);
  if (vkAllocateCommandBuffers(params->logicalDevice, &allocInfo, commandBuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Failed to allocate secondary command buffers");
    return;
  }

  VkCommandBufferInheritanceInfo inheritanceInfo = {
      .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
      .renderPass  = params->renderPass,
      .subpass     = 0,
      .framebuffer = params->framebuffer};
  vkBeginCommandBuffer(*commandBuffer,
      &(VkCommandBufferBeginInfo){
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
          .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
                 | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
          .pInheritanceInfo = &inheritanceInfo});

  VkDescriptorPool descriptorPool = *(VkDescriptorPool*) auto_array_get(
      &params->renderFrame->descriptorPools, threadId);
  vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      params->gBufferPipeline->pipeline);
  g_buffer_pipeline_write_vp(*commandBuffer, descriptorPool,
      params->logicalDevice, &params->renderFrame->vpBuffer,
      params->gBufferPipeline);
  VkViewport viewport = {.x = 0.0f,
      .y                    = 0.0f,
      .width                = (float) params->imageSize.width,
      .height               = (float) params->imageSize.height,
      .minDepth             = 0.0f,
      .maxDepth             = 1.0f};
  vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, params->imageSize};
  vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);

  RenderCommand* meshCommand = auto_array_get(
      &params->renderFrame->renderQueue, params->meshCommandIndex);
  g_buffer_pipeline_write_material(*commandBuffer, descriptorPool,
      params->logicalDevice, meshCommand->material, params->gBufferPipeline);
  for (size_t i = params->meshCommandIndex;
       i < params->meshCommandIndex + params->meshCommandCount; i++)
  {
    meshCommand = auto_array_get(&params->renderFrame->renderQueue, i);
    render_frame_draw_mesh(meshCommand, *commandBuffer, params->gBufferPipeline,
        params->physicalDevice, params->logicalDevice);
  }

  vkEndCommandBuffer(*commandBuffer);
}

static void render_frame_render_g_buffer(RenderFrame* renderFrame,
    VkRenderPass renderPass, GBufferPipeline* gBufferPipeline,
    Transform* camera, RenderStack* renderStack, VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice)
{
  ViewProjection vp = {0};
  mat4_identity(vp.view);
  mat4_translate(
      vp.view, -camera->position.x, -camera->position.y, -camera->position.z);
  mat4_rotate(
      vp.view, -camera->rotation.x, -camera->rotation.y, -camera->rotation.z);
  projection_create_perspective(vp.projection, 90.0f,
      (float) renderStack->lightingPass.imageSize.width
          / (float) renderStack->lightingPass.imageSize.height,
      1000.0f, 0.1f);

  if (!gpu_buffer_write(&renderFrame->vpBuffer, (uint8_t*) &vp,
          sizeof(ViewProjection), 0, logicalDevice))
  {
    LOG_ERROR("WARN: Unable to write VP buffer");
    return;
  }

  profiler_clock_start("sort_meshes");
  qsort(renderFrame->renderQueue.buffer, renderFrame->renderQueue.size,
      sizeof(RenderCommand),
      (int (*)(const void*, const void*)) render_command_compare);
  profiler_clock_end("sort_meshes");

  profiler_clock_start("render_meshes");

  uint32_t lastMaterialIndex = 0;
  for (uint32_t i = 0; i < renderFrame->renderQueue.size; i++)
  {
    RenderCommand* meshCommand = auto_array_get(&renderFrame->renderQueue, i);
    RenderCommand* nextMeshCommand =
        renderFrame->renderQueue.size > i + 1
            ? auto_array_get(&renderFrame->renderQueue, i + 1)
            : NULL;

    if (nextMeshCommand == NULL
        || meshCommand->material != nextMeshCommand->material)
    {
      RecordGBufferCommandsParams* params =
          auto_array_allocate(&renderFrame->recordCommands);
      params->renderFrame      = renderFrame;
      params->meshCommandIndex = lastMaterialIndex;
      params->meshCommandCount = i - lastMaterialIndex + 1;
      params->renderPass       = renderPass;
      params->gBufferPipeline  = gBufferPipeline;
      params->framebuffer      = renderStack->gbufferPass.framebuffer;
      params->logicalDevice    = logicalDevice;
      params->physicalDevice   = physicalDevice;
      params->imageSize        = renderStack->gbufferPass.bufferImages[0].size;

      lastMaterialIndex = i + 1;
    }
  }

  auto_array_allocate_many(
      &renderFrame->recordTasks, renderFrame->recordCommands.size);
  for (size_t i = 0; i < renderFrame->recordTasks.size; i++)
  {
    RecordGBufferCommandsParams* params =
        auto_array_get(&renderFrame->recordCommands, i);
    *(HANDLE*) auto_array_get(&renderFrame->recordTasks, i) =
        task_scheduler_enqueue(
            (TaskFunction) render_frame_record_g_buffer_commands, params, 0);
  }

  for (size_t i = 0; i < renderFrame->recordTasks.size; i++)
  {
    WaitForSingleObject(
        *(HANDLE*) auto_array_get(&renderFrame->recordTasks, i), INFINITE);
  }

  for (int i = 0; i < renderFrame->meshCommandBufferLists.size; i++)
  {
    AutoArray* meshCommandBuffers =
        auto_array_get(&renderFrame->meshCommandBufferLists, i);
    if (meshCommandBuffers->size > 0)
    {
      vkCmdExecuteCommands(renderFrame->gBufferCommandBuffer,
          (uint32_t) meshCommandBuffers->size,
          (VkCommandBuffer*) meshCommandBuffers->buffer);
    }
  }

  profiler_clock_end("render_meshes");
}

static void render_frame_render_lighting(RenderFrame* renderFrame,
    RenderStack* renderStack, PbrPipeline* pbrPipeline, Mesh* fullscreenQuad,
    Transform* camera, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  VkViewport viewport = {.x = 0.0f,
      .y                    = 0.0f,
      .width                = (float) renderStack->lightingPass.imageSize.width,
      .height   = (float) renderStack->lightingPass.imageSize.height,
      .minDepth = 0.0f,
      .maxDepth = 1.0f};
  vkCmdSetViewport(renderFrame->lightingCommandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, renderStack->lightingPass.imageSize};
  vkCmdSetScissor(renderFrame->lightingCommandBuffer, 0, 1, &scissor);

  LightingData lightingData = {.cameraPositionWorldSpace = camera->position};

  if (!gpu_buffer_write(&renderFrame->lightBuffer, (uint8_t*) &lightingData,
          sizeof(lightingData), 0, logicalDevice))
  {
    LOG_ERROR("WARN: Unable to write light buffer");
    return;
  }

  vkCmdBindPipeline(renderFrame->lightingCommandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS, pbrPipeline->pipeline);
  VkDescriptorPool* descriptorPool =
      auto_array_get(&renderFrame->descriptorPools, 0);
  pbr_pipeline_write_descriptor_set(renderFrame->lightingCommandBuffer,
      *descriptorPool, logicalDevice, renderStack, &renderFrame->lightBuffer,
      pbrPipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(renderFrame->lightingCommandBuffer, 0, 1,
      &fullscreenQuad->vertices.buffer, &offset);
  vkCmdBindIndexBuffer(renderFrame->lightingCommandBuffer,
      fullscreenQuad->indices.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(renderFrame->lightingCommandBuffer,
      (uint32_t) fullscreenQuad->indices.size / sizeof(uint16_t), 1, 0, 0, 0);
}

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    RayTracingPipeline* rtPipeline, ShaderBindingTable* sbt,
    Mesh* fullscreenQuad, Transform* camera, VkRenderPass gbufferPass,
    VkRenderPass lightingPass, VkQueue queue, VkCommandPool commandPool,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  for (int i = 0; i < renderFrame->descriptorPools.size; i++)
  {
    VkDescriptorPool* descriptorPool =
        auto_array_get(&renderFrame->descriptorPools, i);
    vkResetDescriptorPool(logicalDevice, *descriptorPool, 0);
  }

  if (!renderFrame->accelerationStructureInitialized)
  {
    if (!acceleration_structure_build(&renderFrame->accelerationStructure,
            &renderFrame->renderQueue, commandPool, logicalDevice,
            physicalDevice))
    {
      // TODO: Handle error
      exit(-1);
    }
    renderFrame->accelerationStructureInitialized = true;
  }

  render_frame_start_pass(renderFrame->gBufferCommandBuffer, gbufferPass,
      renderStack->gbufferPass.framebuffer,
      renderStack->gbufferPass.bufferImages[0].size,
      (const VkClearValue[]){{0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
          {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      NUM_OF_GBUFFER_PASS_LAYERS + 1,
      VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS, logicalDevice);
  render_frame_render_g_buffer(renderFrame, gbufferPass, gBufferPipeline,
      camera, renderStack, logicalDevice, physicalDevice);
  render_frame_end_pass(renderFrame->gBufferCommandBuffer);

  // ******* SHADOWS
  vkResetCommandBuffer(renderFrame->shadowCommandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags            = 0,
      .pInheritanceInfo = NULL};

  if (vkBeginCommandBuffer(renderFrame->shadowCommandBuffer, &beginInfo)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to begin command buffer");
    // TODO: Handle error
    return;
  }

  image_transition_layout(renderFrame->shadowMapInitialized
                              ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                              : VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1, renderFrame->shadowCommandBuffer,
      &renderStack->lightingPass.shadowMap);
  renderFrame->shadowMapInitialized = true;
  image_transition_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1, renderFrame->shadowCommandBuffer,
      &renderStack->gbufferPass.bufferImages[GBL_POSITION]);
  image_transition_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL, 0, 1, 0, 1, renderFrame->shadowCommandBuffer,
      &renderStack->gbufferPass.bufferImages[GBL_NORMAL]);

  vkCmdBindPipeline(renderFrame->shadowCommandBuffer,
      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtPipeline->pipeline);
  ray_tracing_pipeline_write_descriptor_set(renderFrame->shadowCommandBuffer,
      *(VkDescriptorPool*) auto_array_get(&renderFrame->descriptorPools, 0),
      logicalDevice, renderStack, &renderFrame->accelerationStructure,
      rtPipeline);
  _vkCmdTraceRaysKHR(renderFrame->shadowCommandBuffer, &sbt->rgenRegion,
      &sbt->missRegion, &sbt->hitRegion, &sbt->callRegion,
      renderStack->lightingPass.imageSize.width,
      renderStack->lightingPass.imageSize.height, 1);

  image_transition_layout(VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1,
      renderFrame->shadowCommandBuffer, &renderStack->lightingPass.shadowMap);
  image_transition_layout(VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1,
      renderFrame->shadowCommandBuffer,
      &renderStack->gbufferPass.bufferImages[GBL_POSITION]);
  image_transition_layout(VK_IMAGE_LAYOUT_GENERAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1, 0, 1,
      renderFrame->shadowCommandBuffer,
      &renderStack->gbufferPass.bufferImages[GBL_NORMAL]);

  if (vkEndCommandBuffer(renderFrame->shadowCommandBuffer) != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to end recording of command buffer");
    // Handle error
    return;
  }
  // ******* SHADOWS

  render_frame_start_pass(renderFrame->lightingCommandBuffer, lightingPass,
      renderStack->lightingPass.framebuffer,
      renderStack->lightingPass.imageSize,
      (const VkClearValue[]){{}, {}, {}, {}, {}, {0.0, 0.0, 0.0, 1.0}}, 6,
      VK_SUBPASS_CONTENTS_INLINE, logicalDevice);
  render_frame_render_lighting(renderFrame, renderStack, pbrPipeline,
      fullscreenQuad, camera, physicalDevice, logicalDevice);
  render_frame_end_pass(renderFrame->lightingCommandBuffer);

  VkPipelineStageFlags waitStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkPipelineStageFlags rtStages = VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;

  VkSubmitInfo submitInfo[] = {
      {.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount   = 1,
          .pWaitSemaphores      = &renderFrame->imageAvailableSemaphore,
          .pWaitDstStageMask    = &waitStages,
          .commandBufferCount   = 1,
          .pCommandBuffers      = &renderFrame->gBufferCommandBuffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores    = &renderFrame->gBufferFinishedSemaphore},
      {
          .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount   = 1,
          .pWaitSemaphores      = &renderFrame->gBufferFinishedSemaphore,
          .pWaitDstStageMask    = &waitStages,
          .commandBufferCount   = 1,
          .pCommandBuffers      = &renderFrame->shadowCommandBuffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores    = &renderFrame->shadowFinishedSemaphore,
      },
      {.sType                   = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .waitSemaphoreCount   = 1,
          .pWaitSemaphores      = &renderFrame->shadowFinishedSemaphore,
          .pWaitDstStageMask    = &rtStages,
          .commandBufferCount   = 1,
          .pCommandBuffers      = &renderFrame->lightingCommandBuffer,
          .signalSemaphoreCount = 1,
          .pSignalSemaphores    = &renderFrame->renderFinishedSemaphore}};

  if (vkQueueSubmit(
          queue, _countof(submitInfo), submitInfo, renderFrame->inflightFence)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to submit graphics work");
    // TODO: Handle error
    return;
  }

  auto_array_clear(&renderFrame->renderQueue);
}

void render_frame_clear_buffers(
    RenderFrame* renderFrame, VkDevice logicalDevice)
{
  for (uint32_t i = 0; i < renderFrame->perRenderBuffers.size; i++)
  {
    gpu_buffer_free(
        auto_array_get(&renderFrame->perRenderBuffers, i), logicalDevice);
  }
  auto_array_clear(&renderFrame->perRenderBuffers);

  for (int i = 0; i < renderFrame->meshCommandBufferLists.size; i++)
  {
    AutoArray* meshCommandBuffers =
        auto_array_get(&renderFrame->meshCommandBufferLists, i);
    VkCommandPool* commandPool =
        auto_array_get(&renderFrame->secondaryCommandPools, i);
    if (meshCommandBuffers->size > 0)
    {
      vkFreeCommandBuffers(logicalDevice, *commandPool,
          meshCommandBuffers->size, meshCommandBuffers->buffer);
    }

    auto_array_clear(&renderFrame->recordTasks);
    auto_array_clear(&renderFrame->recordCommands);

    auto_array_clear(meshCommandBuffers);
    vkResetCommandPool(logicalDevice, *commandPool, 0);
  }
}

