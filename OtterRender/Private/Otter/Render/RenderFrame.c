#include "Otter/Render/RenderFrame.h"

#include "Otter/Math/Projection.h"
#include "Otter/Render/Raytracing/BoundingVolumeHierarchy.h"
#include "Otter/Render/Raytracing/Ray.h"
#include "Otter/Render/Uniform/ModelViewProjection.h"
#include "Otter/Util/Profiler.h"

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

  bounding_volume_hierarchy_create(&renderFrame->bvh);

  return true;
}

void render_frame_destroy(
    RenderFrame* renderFrame, VkCommandPool commandPool, VkDevice logicalDevice)
{
  bounding_volume_hierarchy_destroy(&renderFrame->bvh);

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

static void render_frame_initialize_bvh(RenderFrame* renderFrame)
{
  profiler_clock_start("bvh_init");
  bounding_volume_hierarchy_reset(&renderFrame->bvh);
  for (uint32_t i = 0; i < renderFrame->renderQueue.size; i++)
  {
    RenderCommand* meshCommand = auto_array_get(&renderFrame->renderQueue, i);
    bounding_volume_hierarchy_add_primitives(meshCommand->cpuVertices,
        meshCommand->numOfVertices, meshCommand->cpuIndices,
        meshCommand->numOfIndices, &meshCommand->transform, &renderFrame->bvh);
  }
  profiler_clock_end("bvh_init");
  profiler_clock_start("bvh_build");
  bounding_volume_hierarchy_build(&renderFrame->bvh);
  profiler_clock_end("bvh_build");
}

typedef struct RenderShadowKernel
{
  VkExtent2D screenSize;
  Vec2 unitSize;
  float view;
  float aspectRatio;
  Vec3* camera;
  BoundingVolumeHierarchy* bvh;
  float* shadowMap;
  uint32_t startHeight;
  uint32_t endHeight;
} RenderShadowKernel;

static DWORD WINAPI render_frame_draw_shadow_thread(RenderShadowKernel* kernel)
{
  for (uint32_t y = kernel->startHeight; y < kernel->endHeight; y++)
  {
    for (uint32_t x = 0; x < kernel->screenSize.width; x++)
    {
      Vec3 worldSpace = {2.0f * (x + 0.5f) * kernel->unitSize.x - 1.0f,
          1.0f - 2.0f * (y + 0.5f) * kernel->unitSize.y};
      worldSpace.x *= kernel->view * kernel->aspectRatio;
      worldSpace.y *= kernel->view;
      worldSpace.z = -1;

      vec3_normalize(&worldSpace);

      Ray primaryRay;
      ray_create(&primaryRay, kernel->camera, &worldSpace);

      Vec3 primaryHit;
      if (ray_cast(&primaryRay, kernel->bvh, &primaryHit))
      {
        kernel->shadowMap[x + y * kernel->screenSize.width] = 1.0f;
      }
      else
      {
        kernel->shadowMap[x + y * kernel->screenSize.width] = 0.0f;
      }
    }
  }
  return 0;
}

#define MAXTHREADS 16

static void render_frame_draw_shadows(
    RenderStack* renderStack, BoundingVolumeHierarchy* bvh, Vec3* camera)
{
  float* shadowMap            = renderStack->cpuShadowMapBuffer.mapped;
  const VkExtent2D screenSize = renderStack->cpuShadowMap.size;
  const Vec2 unitSize     = {1.0f / screenSize.width, 1.0f / screenSize.height};
  const float aspectRatio = (float) screenSize.width / screenSize.height;
  const float fieldOfView = 90.0f;
  const float view        = tanf((float) M_PI * 0.5f * fieldOfView / 180.0f);

  HANDLE threads[MAXTHREADS];
  RenderShadowKernel* threadData[MAXTHREADS];
  for (int i = 0; i < MAXTHREADS; i++)
  {
    threadData[i]              = malloc(sizeof(RenderShadowKernel));
    threadData[i]->screenSize  = screenSize;
    threadData[i]->unitSize    = unitSize;
    threadData[i]->view        = view;
    threadData[i]->aspectRatio = aspectRatio;
    threadData[i]->camera      = camera;
    threadData[i]->bvh         = bvh;
    threadData[i]->shadowMap   = shadowMap;
    threadData[i]->startHeight = i * screenSize.height / MAXTHREADS;
    threadData[i]->endHeight   = (i + 1) * screenSize.height / MAXTHREADS;

    threads[i] = CreateThread(
        NULL, 0, render_frame_draw_shadow_thread, threadData[i], 0, NULL);
  }

  WaitForMultipleObjects(MAXTHREADS, threads, TRUE, INFINITE);
}

static void render_frame_submit_cpu_frames(RenderFrame* renderFrame,
    RenderStack* renderStack, VkCommandPool commandPool, VkDevice logicalDevice,
    VkQueue queue)
{
  VkCommandBufferAllocateInfo transferBufferAllocateInfo = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
  VkCommandBuffer transferBuffer;
  if (vkAllocateCommandBuffers(
          logicalDevice, &transferBufferAllocateInfo, &transferBuffer)
      != VK_SUCCESS)
  {
    // TODO: Handle
    return;
  }

  // TODO: Try prebaking.
  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  if (vkBeginCommandBuffer(transferBuffer, &beginInfo) != VK_SUCCESS)
  {
    // TODO: Handle.
    return;
  }

  VkImageMemoryBarrier barrier = {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout           = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .srcAccessMask       = 0,
      .dstAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
      .image               = renderStack->cpuShadowMap.image,
      .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
             .baseMipLevel                = 0,
             .levelCount                  = 1,
             .baseArrayLayer              = 0,
             .layerCount                  = 1}};

  vkCmdPipelineBarrier(transferBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

  VkBufferImageCopy region = {
      .imageSubresource = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount                  = 1},
      .imageExtent      = {renderStack->cpuShadowMap.size.width,
               renderStack->cpuShadowMap.size.height, 1}};

  vkCmdCopyBufferToImage(transferBuffer, renderStack->cpuShadowMapBuffer.buffer,
      renderStack->cpuShadowMap.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
      &region);

  VkImageMemoryBarrier barrierReturn = {
      .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout           = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask       = VK_ACCESS_SHADER_READ_BIT,
      .image               = renderStack->cpuShadowMap.image,
      .subresourceRange    = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
             .baseMipLevel                = 0,
             .levelCount                  = 1,
             .baseArrayLayer              = 0,
             .layerCount                  = 1}};

  vkCmdPipelineBarrier(transferBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1,
      &barrierReturn);

  if (vkEndCommandBuffer(transferBuffer) != VK_SUCCESS)
  {
    // TODO: Handle.
    return;
  }

  VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount           = 1,
      .pCommandBuffers              = &transferBuffer};

  if (vkQueueSubmit(queue, 1, &submitInfo, NULL) != VK_SUCCESS)
  {
    fprintf(stderr, "Error: Unable to submit graphics work\n");
    // TODO: Handle error
    return;
  }

  // TODO: Add synchonization and separate this into a copy queue.
  // Multithreading people, multithreading.
  if (vkDeviceWaitIdle(logicalDevice) != VK_SUCCESS)
  {
    // TODO: Handle.
    return;
  }

  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferBuffer);
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
    fprintf(stderr, "Error: Unable to begin command buffer\n");
    // TODO: Handle error
    return;
  }

  VkClearValue clearColor[NUM_OF_RENDER_STACK_LAYERS] = {
      {0.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
      {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f}};
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

static void render_frame_render_g_buffer(RenderFrame* renderFrame,
    GBufferPipeline* gBufferPipeline, Vec3* camera, RenderStack* renderStack,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
{
  vkCmdBindPipeline(renderFrame->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      gBufferPipeline->pipeline);

  ModelViewProjection mvp = {0};
  mat4_identity(mvp.view);
  mat4_translate(mvp.view, -camera->x, camera->y, -camera->z);
  projection_create_perspective(mvp.projection, 90.0f,
      (float) renderStack->gBufferImage.size.width
          / (float) renderStack->gBufferImage.size.height,
      0.1f, 100.0f);

  for (uint32_t i = 0; i < renderFrame->renderQueue.size; i++)
  {
    RenderCommand* meshCommand = auto_array_get(&renderFrame->renderQueue, i);
    render_frame_draw_mesh(meshCommand, renderFrame, &mvp, gBufferPipeline,
        physicalDevice, logicalDevice);
  }
}

static void render_frame_render_lighting(RenderFrame* renderFrame,
    RenderStack* renderStack, PbrPipeline* pbrPipeline, Mesh* fullscreenQuad,
    VkDevice logicalDevice)
{
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
}

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    GBufferPipeline* gBufferPipeline, PbrPipeline* pbrPipeline,
    Mesh* fullscreenQuad, Vec3* camera, VkRenderPass renderPass, VkQueue queue,
    VkCommandPool commandPool, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  profiler_clock_start("bvh_create");
  render_frame_initialize_bvh(renderFrame);
  profiler_clock_end("bvh_create");

  profiler_clock_start("cpu_shadow_rt");
  render_frame_draw_shadows(renderStack, &renderFrame->bvh, camera);
  profiler_clock_end("cpu_shadow_rt");

  profiler_clock_start("cpu_buffer_copy");
  render_frame_submit_cpu_frames(
      renderFrame, renderStack, commandPool, logicalDevice, queue);
  profiler_clock_end("cpu_buffer_copy");

  profiler_clock_start("render_submit");
  render_frame_start_render(
      renderFrame, renderPass, renderStack, logicalDevice);
  render_frame_render_g_buffer(renderFrame, gBufferPipeline, camera,
      renderStack, logicalDevice, physicalDevice);

  vkCmdNextSubpass(renderFrame->commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

  render_frame_render_lighting(
      renderFrame, renderStack, pbrPipeline, fullscreenQuad, logicalDevice);

  render_frame_end_render(renderFrame, queue);
  profiler_clock_end("render_submit");
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
