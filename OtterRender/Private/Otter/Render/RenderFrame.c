#include "Otter/Render/RenderFrame.h"

#include "Otter/Math/Projection.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Render/Uniform/ViewProjection.h"
#include "Otter/Util/Profiler.h"

#define DESCRIPTOR_POOL_SIZE 128
#define DESCRIPTOR_SET_LIMIT 16 * 1024

bool render_frame_create(
    RenderFrame* renderFrame, VkDevice logicalDevice, VkCommandPool commandPool)
{
  memset(renderFrame, 0, sizeof(RenderFrame));

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

  VkDescriptorPoolCreateInfo createInfo = {
      .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets       = DESCRIPTOR_SET_LIMIT,
      .poolSizeCount = _countof(poolSizes),
      .pPoolSizes    = poolSizes};

  if (vkCreateDescriptorPool(
          logicalDevice, &createInfo, NULL, &renderFrame->descriptorPool)
      != VK_SUCCESS)
  {
    LOG_ERROR("Could not create descriptor pools");
    return false;
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

  if (renderFrame->descriptorPool != VK_NULL_HANDLE)
  {
    vkDestroyDescriptorPool(logicalDevice, renderFrame->descriptorPool, NULL);
  }
}

static void render_frame_draw_mesh(RenderCommand* meshCommand,
    RenderFrame* renderFrame, GBufferPipeline* gBufferPipeline,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  vkCmdPushConstants(renderFrame->commandBuffer, gBufferPipeline->layout,
      VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Mat4), &meshCommand->transform);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(renderFrame->commandBuffer, 0, 1,
      &meshCommand->mesh->vertices.buffer, &offset);
  vkCmdBindIndexBuffer(renderFrame->commandBuffer,
      meshCommand->mesh->indices.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(renderFrame->commandBuffer,
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

  vkCmdBeginRenderPass(
      renderFrame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {.x = 0.0f,
      .y                    = 0.0f,
      .width                = (float) renderStack->gBufferImage.size.width,
      .height               = (float) renderStack->gBufferImage.size.height,
      .minDepth             = 0.0f,
      .maxDepth             = 1.0f};
  vkCmdSetViewport(renderFrame->commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, renderStack->gBufferImage.size};
  vkCmdSetScissor(renderFrame->commandBuffer, 0, 1, &scissor);

  vkResetDescriptorPool(logicalDevice, renderFrame->descriptorPool, 0);
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

static void render_frame_render_g_buffer(RenderFrame* renderFrame,
    GBufferPipeline* gBufferPipeline, Transform* camera,
    RenderStack* renderStack, VkDevice logicalDevice,
    VkPhysicalDevice physicalDevice)
{
  vkCmdBindPipeline(renderFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      gBufferPipeline->pipeline);

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

  g_buffer_pipeline_write_vp(renderFrame->commandBuffer,
      renderFrame->descriptorPool, logicalDevice, vpBuffer, gBufferPipeline);

  profiler_clock_start("render_meshes");
  for (uint32_t i = 0; i < renderFrame->renderQueue.size; i++)
  {
    RenderCommand* meshCommand = auto_array_get(&renderFrame->renderQueue, i);
    g_buffer_pipeline_write_material(renderFrame->commandBuffer,
        renderFrame->descriptorPool, logicalDevice, &meshCommand->material,
        gBufferPipeline);
    render_frame_draw_mesh(meshCommand, renderFrame, gBufferPipeline,
        physicalDevice, logicalDevice);
  }
  profiler_clock_end("render_meshes");
}

static void render_frame_render_lighting(RenderFrame* renderFrame,
    RenderStack* renderStack, PbrPipeline* pbrPipeline, Mesh* fullscreenQuad,
    Transform* camera, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
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
  pbr_pipeline_write_descriptor_set(renderFrame->commandBuffer,
      renderFrame->descriptorPool, logicalDevice, renderStack, pbrData,
      pbrPipeline);

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
  render_frame_render_g_buffer(renderFrame, gBufferPipeline, camera,
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
}
 
