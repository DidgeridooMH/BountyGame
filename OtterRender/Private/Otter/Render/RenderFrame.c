#include "Otter/Render/RenderFrame.h"

#include "Otter/Render/Uniform/ModelViewProjection.h"

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

  return true;
}

void render_frame_destroy(
    RenderFrame* renderFrame, VkCommandPool commandPool, VkDevice logicalDevice)
{
  if (renderFrame->perRenderBuffersCount > 0)
  {
    for (uint32_t i = 0; i < renderFrame->perRenderBuffersCount; i++)
    {
      gpu_buffer_free(&renderFrame->perRenderBuffers[i], logicalDevice);
    }
  }
  free(renderFrame->perRenderBuffers);

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

void render_frame_draw(RenderFrame* renderFrame, RenderStack* renderStack,
    RenderCommand* command, GBufferPipeline* gBufferPipeline,
    VkExtent2D extents, VkRenderPass renderPass, VkQueue queue,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
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

  VkClearValue clearColor[]            = {{0.0f, 0.0f, 0.0f, 1.0f},
                 {0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f},
                 {0.0f, 0.0f, 0.0f, 1.0f}};
  VkRenderPassBeginInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass      = renderPass,
      .framebuffer     = renderStack->gBuffer,
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

  // TODO: Render meshes that are queued up.
  ModelViewProjection mvp = {0};
  mat4_identity(mvp.model);
  mat4_identity(mvp.view);
  mat4_identity(mvp.projection);

  //****
  // TODO: Abstract to an auto array

  if (renderFrame->perRenderBuffersCount
      == renderFrame->perRenderBuffersCapacity)
  {
    renderFrame->perRenderBuffersCapacity += 32;
    // TODO: deal with realloc failing.
    renderFrame->perRenderBuffers = realloc(
        renderFrame->perRenderBuffers, renderFrame->perRenderBuffersCapacity);
  }

  if (!gpu_buffer_allocate(
          &renderFrame->perRenderBuffers[renderFrame->perRenderBuffersCount],
          sizeof(ModelViewProjection), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    fprintf(stderr, "Warning: Could not allocate temporary gpu buffer...but I "
                    "also don't know what to do about that.\n");
    return;
  }
  GpuBuffer* mvpBuffer =
      &renderFrame->perRenderBuffers[renderFrame->perRenderBuffersCount];
  renderFrame->perRenderBuffersCount += 1;
  //****

  if (!gpu_buffer_write(
          mvpBuffer, (uint8_t*) &mvp, sizeof(mvp), 0, logicalDevice))
  {
    fprintf(stderr, "WARN: Unable to write MVP buffer\n");
    gpu_buffer_free(mvpBuffer, logicalDevice);
    mvpBuffer = NULL;
    return;
  }

  VkDescriptorSetAllocateInfo mvpDescriptorSetAllocInfo = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = renderFrame->descriptorPool,
      .pSetLayouts        = &gBufferPipeline->descriptorSetLayouts,
      .descriptorSetCount = 1};
  VkDescriptorSet mvpDescriptorSet;
  if (vkAllocateDescriptorSets(
          logicalDevice, &mvpDescriptorSetAllocInfo, &mvpDescriptorSet)
      != VK_SUCCESS)
  {
    fprintf(stderr, "WARN: Unable to allocate descriptors\n");
  }

  VkDescriptorBufferInfo mvpBufferInfo = {
      .buffer = mvpBuffer->buffer, .offset = 0, .range = mvpBuffer->size};
  VkWriteDescriptorSet mvpWrite = {
      .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      .descriptorCount = 1,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .dstSet          = mvpDescriptorSet,
      .dstBinding      = 0,
      .dstArrayElement = 0,
      .pBufferInfo     = &mvpBufferInfo};
  vkUpdateDescriptorSets(logicalDevice, 1, &mvpWrite, 0, NULL);

  vkCmdBindDescriptorSets(renderFrame->commandBuffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS, gBufferPipeline->layout, 0, 1,
      &mvpDescriptorSet, 0, NULL);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      renderFrame->commandBuffer, 0, 1, &command->vertices, &offset);
  vkCmdBindIndexBuffer(
      renderFrame->commandBuffer, command->indices, 0, VK_INDEX_TYPE_UINT16);
  vkCmdDrawIndexed(
      renderFrame->commandBuffer, (uint32_t) command->numOfIndices, 1, 0, 0, 0);

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
}
