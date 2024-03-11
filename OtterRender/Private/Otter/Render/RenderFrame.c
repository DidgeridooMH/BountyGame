#include "Otter/Render/RenderFrame.h"

#include "Otter/Render/Uniform/ModelViewProjection.h"
#include "Otter/Math/Projection.h"

#define DESCRIPTOR_POOL_SIZE 128
#define DESCRIPTOR_SET_LIMIT 512

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
    fprintf(stderr, "Error: Failed to allocate command buffers\n");
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
    fprintf(stderr, "Could not create descriptor pools\n");
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
    fprintf(stderr, "Error: Failed to create synchronization objects\n");
    return false;
  }

  auto_array_create(&renderFrame->renderQueue, sizeof(RenderCommand));
  auto_array_create(&renderFrame->perRenderBuffers, sizeof(GpuBuffer));

  return true;
}

void render_frame_destroy(
    RenderFrame* renderFrame, VkCommandPool commandPool, VkDevice logicalDevice)
{
  for (uint32_t i = 0; i < renderFrame->perRenderBuffers.size; i++)
  {
    gpu_buffer_free(
        auto_array_get(&renderFrame->perRenderBuffers, i), logicalDevice);
  }

  auto_array_destroy(&renderFrame->perRenderBuffers);
  auto_array_destroy(&renderFrame->renderQueue);

  if (renderFrame->inflightFence != VK_NULL_HANDLE)
  {
    vkWaitForFences(
        logicalDevice, 1, &renderFrame->inflightFence, true, UINT64_MAX);
  }

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
    RenderFrame* renderFrame, ModelViewProjection* mvp,
    GBufferPipeline* gBufferPipeline, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  mat4_identity(mvp->model);
  transform_apply(mvp->model, &meshCommand->transform);

  GpuBuffer* mvpBuffer = auto_array_allocate(&renderFrame->perRenderBuffers);
  if (mvpBuffer == NULL)
  {
    fprintf(stderr, "Unable to queue mesh for rendering.\n");
    return;
  }

  if (!gpu_buffer_allocate(mvpBuffer, sizeof(ModelViewProjection),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    fprintf(stderr, "Warning: Could not allocate temporary gpu buffer...but I "
                    "also don't know what to do about that.\n");
    return;
  }

  if (!gpu_buffer_write(mvpBuffer, (uint8_t*) mvp, sizeof(ModelViewProjection),
          0, logicalDevice))
  {
    fprintf(stderr, "WARN: Unable to write MVP buffer\n");
    gpu_buffer_free(mvpBuffer, logicalDevice);
    mvpBuffer = NULL;
    return;
  }

  g_buffer_pipeline_write_descriptor_set(renderFrame->commandBuffer,
      renderFrame->descriptorPool, logicalDevice, mvpBuffer, gBufferPipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      renderFrame->commandBuffer, 0, 1, &meshCommand->vertices, &offset);
  vkCmdBindIndexBuffer(renderFrame->commandBuffer, meshCommand->indices, 0,
      VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(renderFrame->commandBuffer,
      (uint32_t) meshCommand->numOfIndices, 1, 0, 0, 0);
}

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    Mesh* fullscreenQuad, Vec3* camera, VkExtent2D extents,
    VkRenderPass renderPass, VkQueue queue, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  vkResetCommandBuffer(renderFrame->commandBuffer, 0);

  VkCommandBufferBeginInfo beginInfo = {
      .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags            = 0,
      .pInheritanceInfo = NULL};

  if (vkBeginCommandBuffer(renderFrame->commandBuffer, &beginInfo)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to begin command buffer\n");
    // TODO: Handle error
    return;
  }

  VkClearValue clearColor[]            = {{0.0f, 0.0f, 0.0f, 0.0f},
                 {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
                 {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}};
  VkRenderPassBeginInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass      = renderPass,
      .framebuffer     = renderStack->framebuffer,
      .renderArea      = {{0, 0}, extents},
      .clearValueCount = _countof(clearColor),
      .pClearValues    = clearColor};

  vkCmdBeginRenderPass(
      renderFrame->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {.x = 0.0f,
      .y                    = 0.0f,
      .width                = (float) extents.width,
      .height               = (float) extents.height,
      .minDepth             = 0.0f,
      .maxDepth             = 1.0f};
  vkCmdSetViewport(renderFrame->commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {{0, 0}, extents};
  vkCmdSetScissor(renderFrame->commandBuffer, 0, 1, &scissor);

  vkResetDescriptorPool(logicalDevice, renderFrame->descriptorPool, 0);

  vkCmdBindPipeline(renderFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      gBufferPipeline->pipeline);

  ModelViewProjection mvp = {0};
  mat4_identity(mvp.view);
  mat4_translate(mvp.view, -camera->x, camera->y, -camera->z);
  projection_create_perspective(mvp.projection, 90.0f,
      (float) extents.width / (float) extents.height, 0.1f, 100.0f);

  for (uint32_t i = 0; i < renderFrame->renderQueue.size; i++)
  {
    render_frame_draw_mesh(auto_array_get(&renderFrame->renderQueue, i),
        renderFrame, &mvp, gBufferPipeline, physicalDevice, logicalDevice);
  }

  // Lighting subpass
  vkCmdNextSubpass(renderFrame->commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(renderFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pbrPipeline->pipeline);
  pbr_pipeline_write_descriptor_set(renderFrame->commandBuffer,
      renderFrame->descriptorPool, logicalDevice, renderStack, pbrPipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(renderFrame->commandBuffer, 0, 1,
      &fullscreenQuad->vertices.buffer, &offset);
  vkCmdBindIndexBuffer(renderFrame->commandBuffer,
      fullscreenQuad->indices.buffer, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(renderFrame->commandBuffer,
      (uint32_t) fullscreenQuad->indices.size / sizeof(uint16_t), 1, 0, 0, 0);

  vkCmdEndRenderPass(renderFrame->commandBuffer);

  if (vkEndCommandBuffer(renderFrame->commandBuffer) != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to end recording of command buffer\n");
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
    fprintf(stderr, "Error: Unable to submit graphics work\n");
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
}
