#include "Otter/Render/RenderFrame.h"

#include "Otter/Async/Scheduler.h"
#include "Otter/Math/Projection.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/Uniform/ViewProjection.h"
#include "Otter/Util/AutoArray.h"
#include "Otter/Util/Profiler.h"

#define DESCRIPTOR_POOL_SIZE 64
#define DESCRIPTOR_SET_LIMIT 8 * 1024

bool render_frame_create(RenderFrame* renderFrame, uint32_t graphicsQueueFamily,
    VkDevice logicalDevice, VkCommandPool commandPool)
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
          logicalDevice, &allocInfo, &renderFrame->commandBuffer)
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
             &renderFrame->renderFinishedSemaphore)
             != VK_SUCCESS
      || vkCreateFence(
             logicalDevice, &fenceInfo, NULL, &renderFrame->inflightFence)
             != VK_SUCCESS)
  {
    LOG_ERROR("Error: Failed to create synchronization objects");
    return false;
  }

  auto_array_create(&renderFrame->renderQueue, sizeof(RenderCommand));
  auto_array_create(&renderFrame->perRenderBuffers, sizeof(GpuBuffer));

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

  auto_array_destroy(&renderFrame->perRenderBuffers);
  auto_array_destroy(&renderFrame->renderQueue);

  vkDestroySemaphore(logicalDevice, renderFrame->imageAvailableSemaphore, NULL);
  vkDestroySemaphore(logicalDevice, renderFrame->renderFinishedSemaphore, NULL);
  vkDestroyFence(logicalDevice, renderFrame->inflightFence, NULL);

  if (renderFrame->commandBuffer)
  {
    vkFreeCommandBuffers(
        logicalDevice, commandPool, 1, &renderFrame->commandBuffer);
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

static void render_frame_start_render(RenderFrame* renderFrame,
    VkRenderPass renderPass, RenderStack* renderStack, VkDevice logicalDevice)
{
  vkResetCommandBuffer(renderFrame->commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags            = 0,
      .pInheritanceInfo = NULL};

  if (vkBeginCommandBuffer(renderFrame->commandBuffer, &beginInfo)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to begin command buffer");
    // TODO: Handle error
    return;
  }

  VkClearValue clearColor[NUM_OF_RENDER_STACK_LAYERS] = {
      {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}};
  VkRenderPassBeginInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass      = renderPass,
      .framebuffer     = renderStack->framebuffer,
      .renderArea      = {{0, 0}, renderStack->gBufferImage.size},
      .clearValueCount = _countof(clearColor),
      .pClearValues    = clearColor};

  vkCmdBeginRenderPass(renderFrame->commandBuffer, &renderPassInfo,
      VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

  for (int i = 0; i < renderFrame->descriptorPools.size; i++)
  {
    VkDescriptorPool* descriptorPool =
        auto_array_get(&renderFrame->descriptorPools, i);
    vkResetDescriptorPool(logicalDevice, *descriptorPool, 0);
  }
}

static void render_frame_end_render(RenderFrame* renderFrame, VkQueue queue)
{
  vkCmdEndRenderPass(renderFrame->commandBuffer);

  if (vkEndCommandBuffer(renderFrame->commandBuffer) != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to end recording of command buffer");
    // Handle error
    return;
  }

  VkPipelineStageFlags waitStages =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount           = 1,
      .pWaitSemaphores              = &renderFrame->imageAvailableSemaphore,
      .pWaitDstStageMask            = &waitStages,
      .commandBufferCount           = 1,
      .pCommandBuffers              = &renderFrame->commandBuffer,
      .signalSemaphoreCount         = 1,
      .pSignalSemaphores            = &renderFrame->renderFinishedSemaphore};

  if (vkQueueSubmit(queue, 1, &submitInfo, renderFrame->inflightFence)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to submit graphics work");
    // TODO: Handle error
    return;
  }

  auto_array_clear(&renderFrame->renderQueue);
}

typedef struct RecordGBufferCommandsParams
{
  RenderFrame* renderFrame;
  RenderCommand* meshCommand;
  VkRenderPass renderPass;
  GBufferPipeline* gBufferPipeline;
  VkFramebuffer framebuffer;
  VkDevice logicalDevice;
  VkPhysicalDevice physicalDevice;
  GpuBuffer* vpBuffer;
  VkExtent2D imageSize;
} RecordGBufferCommandsParams;

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
      params->logicalDevice, params->vpBuffer, params->gBufferPipeline);
  VkViewport viewport = {.x = 0.0f,
      .y                    = 0.0f,
      .width                = (float) params->imageSize.width,
      .height               = (float) params->imageSize.height,
      .minDepth             = 0.0f,
      .maxDepth             = 1.0f};
  vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, params->imageSize};
  vkCmdSetScissor(*commandBuffer, 0, 1, &scissor);

  g_buffer_pipeline_write_material(*commandBuffer, descriptorPool,
      params->logicalDevice, &params->meshCommand->material,
      params->gBufferPipeline);
  render_frame_draw_mesh(params->meshCommand, *commandBuffer,
      params->gBufferPipeline, params->physicalDevice, params->logicalDevice);

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
      (float) renderStack->gBufferImage.size.width
          / (float) renderStack->gBufferImage.size.height,
      1000.0f, 0.1f);

  // TODO: make this a one time allocation in the instance/frame.
  GpuBuffer* vpBuffer = auto_array_allocate(&renderFrame->perRenderBuffers);
  if (vpBuffer == NULL)
  {
    LOG_ERROR("Unable to allocate VP buffer");
    return;
  }

  if (!gpu_buffer_allocate(vpBuffer, sizeof(ViewProjection),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    LOG_ERROR("Warning: Could not allocate temporary gpu buffer...but I "
              "also don't know what to do about that.");
    return;
  }

  if (!gpu_buffer_write(
          vpBuffer, (uint8_t*) &vp, sizeof(ViewProjection), 0, logicalDevice))
  {
    LOG_ERROR("WARN: Unable to write VP buffer");
    gpu_buffer_free(vpBuffer, logicalDevice);
    return;
  }

  profiler_clock_start("render_meshes");

  AutoArray recordTasks;
  auto_array_create(&recordTasks, sizeof(HANDLE));
  auto_array_allocate_many(&recordTasks, renderFrame->renderQueue.size);

  AutoArray recordCommands;
  auto_array_create(&recordCommands, sizeof(RecordGBufferCommandsParams));
  auto_array_allocate_many(&recordCommands, renderFrame->renderQueue.size);
  for (uint32_t i = 0; i < renderFrame->renderQueue.size; i++)
  {
    RenderCommand* meshCommand = auto_array_get(&renderFrame->renderQueue, i);

    RecordGBufferCommandsParams* params = auto_array_get(&recordCommands, i);
    params->renderFrame                 = renderFrame;
    params->meshCommand                 = meshCommand;
    params->renderPass                  = renderPass;
    params->gBufferPipeline             = gBufferPipeline;
    params->framebuffer                 = renderStack->framebuffer;
    params->logicalDevice               = logicalDevice;
    params->physicalDevice              = physicalDevice;
    params->vpBuffer                    = vpBuffer;
    params->imageSize                   = renderStack->gBufferImage.size;

    *(HANDLE*) auto_array_get(&recordTasks, i) = task_scheduler_enqueue(
        (TaskFunction) render_frame_record_g_buffer_commands, params, 0);
  }

  for (size_t i = 0; i < recordTasks.size; i++)
  {
    WaitForSingleObject(*(HANDLE*) auto_array_get(&recordTasks, i), INFINITE);
  }

  auto_array_destroy(&recordCommands);
  auto_array_destroy(&recordTasks);

  for (int i = 0; i < renderFrame->meshCommandBufferLists.size; i++)
  {
    AutoArray* meshCommandBuffers =
        auto_array_get(&renderFrame->meshCommandBufferLists, i);
    if (meshCommandBuffers->size > 0)
    {
      vkCmdExecuteCommands(renderFrame->commandBuffer,
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
      .width                = (float) renderStack->gBufferImage.size.width,
      .height               = (float) renderStack->gBufferImage.size.height,
      .minDepth             = 0.0f,
      .maxDepth             = 1.0f};
  vkCmdSetViewport(renderFrame->commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, renderStack->gBufferImage.size};
  vkCmdSetScissor(renderFrame->commandBuffer, 0, 1, &scissor);

  LightingData lightingData = {.cameraPositionWorldSpace = camera->position};

  GpuBuffer* pbrData = auto_array_allocate(&renderFrame->perRenderBuffers);
  if (pbrData == NULL)
  {
    LOG_ERROR("Unable to allocate lighting data");
    return;
  }

  if (!gpu_buffer_allocate(pbrData, sizeof(lightingData),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    LOG_ERROR("Warning: Could not allocate temporary gpu buffer...but I "
              "also don't know what to do about that.");
    return;
  }

  if (!gpu_buffer_write(pbrData, (uint8_t*) &lightingData, sizeof(lightingData),
          0, logicalDevice))
  {
    LOG_ERROR("WARN: Unable to write MVP buffer");
    gpu_buffer_free(pbrData, logicalDevice);
    return;
  }

  vkCmdBindPipeline(renderFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pbrPipeline->pipeline);
  VkDescriptorPool* descriptorPool =
      auto_array_get(&renderFrame->descriptorPools, 0);
  pbr_pipeline_write_descriptor_set(renderFrame->commandBuffer, *descriptorPool,
      logicalDevice, renderStack, pbrData, pbrPipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(renderFrame->commandBuffer, 0, 1,
      &fullscreenQuad->vertices.buffer, &offset);
  vkCmdBindIndexBuffer(renderFrame->commandBuffer,
      fullscreenQuad->indices.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(renderFrame->commandBuffer,
      (uint32_t) fullscreenQuad->indices.size / sizeof(uint16_t), 1, 0, 0, 0);
}

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    Mesh* fullscreenQuad, Transform* camera, VkRenderPass renderPass,
    VkQueue queue, VkCommandPool commandPool, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  profiler_clock_start("render_start");
  render_frame_start_render(
      renderFrame, renderPass, renderStack, logicalDevice);
  profiler_clock_end("render_start");
  profiler_clock_start("render_g_buffer");
  render_frame_render_g_buffer(renderFrame, renderPass, gBufferPipeline, camera,
      renderStack, logicalDevice, physicalDevice);
  profiler_clock_end("render_g_buffer");

  vkCmdNextSubpass(renderFrame->commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

  profiler_clock_start("render_lighting");
  render_frame_render_lighting(renderFrame, renderStack, pbrPipeline,
      fullscreenQuad, camera, physicalDevice, logicalDevice);
  profiler_clock_end("render_lighting");

  profiler_clock_start("render_end");
  render_frame_end_render(renderFrame, queue);
  profiler_clock_end("render_end");
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
    auto_array_clear(meshCommandBuffers);
    vkResetCommandPool(logicalDevice, *commandPool, 0);
  }
}
