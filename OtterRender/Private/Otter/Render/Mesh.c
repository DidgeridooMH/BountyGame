#include "Otter/Render/Mesh.h"

#include "Otter/Util/Log.h"

Mesh* mesh_create(const void* vertices, uint64_t vertexSize,
    uint64_t numOfVertices, const uint16_t indices[], uint64_t numOfIndices,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandPool commandPool, VkQueue commandQueue)
{
  Mesh* mesh = calloc(1, sizeof(Mesh));
  if (mesh == NULL)
  {
    return NULL;
  }

  GpuBuffer vertexStagingBuffer = {0};
  GpuBuffer indexStagingBuffer  = {0};
  if (!gpu_buffer_allocate(&vertexStagingBuffer, numOfVertices * vertexSize,
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice)
      || !gpu_buffer_allocate(&indexStagingBuffer,
          numOfIndices * sizeof(indices[0]), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  if (!gpu_buffer_allocate(&mesh->vertices, numOfVertices * vertexSize,
          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice)
      || !gpu_buffer_allocate(&mesh->indices, numOfIndices * sizeof(indices[0]),
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice))
  {
    LOG_ERROR("There was a problem allocating the mesh");
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  if (!gpu_buffer_write(&vertexStagingBuffer, (uint8_t*) vertices,
          mesh->vertices.size, 0, logicalDevice)
      || !gpu_buffer_write(&indexStagingBuffer, (uint8_t*) indices,
          mesh->indices.size, 0, logicalDevice))
  {
    LOG_ERROR("There was an issue writing to the buffer.");
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  VkCommandBufferAllocateInfo transferCommandsAlloc = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandPool        = commandPool,
      .commandBufferCount = 1};
  VkCommandBuffer transferCommands;
  if (vkAllocateCommandBuffers(
          logicalDevice, &transferCommandsAlloc, &transferCommands)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to allocate command buffer");
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  if (vkBeginCommandBuffer(transferCommands, &beginInfo) != VK_SUCCESS)
  {
    LOG_ERROR("Unable to start command buffer");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  gpu_buffer_transfer(&vertexStagingBuffer, &mesh->vertices, transferCommands);
  gpu_buffer_transfer(&indexStagingBuffer, &mesh->indices, transferCommands);

  if (vkEndCommandBuffer(transferCommands) != VK_SUCCESS)
  {
    LOG_ERROR("Unable to end command buffer");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pCommandBuffers              = &transferCommands,
      .commandBufferCount           = 1};
  if (vkQueueSubmit(commandQueue, 1, &submitInfo, NULL) != VK_SUCCESS)
  {
    LOG_ERROR("Unable to submit queue");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  if (vkQueueWaitIdle(commandQueue) != VK_SUCCESS)
  {
    LOG_ERROR("Unable to start command buffer");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(&indexStagingBuffer, logicalDevice);
    mesh_destroy(mesh, logicalDevice);
    return NULL;
  }

  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);

  gpu_buffer_free(&vertexStagingBuffer, logicalDevice);
  gpu_buffer_free(&indexStagingBuffer, logicalDevice);

  // TODO: Kill me.
  mesh->cpuVertices = malloc(sizeof(MeshVertex) * numOfVertices);
  memcpy(mesh->cpuVertices, vertices, sizeof(MeshVertex) * numOfVertices);
  mesh->cpuIndices = malloc(sizeof(uint16_t) * numOfIndices);
  memcpy(mesh->cpuIndices, indices, sizeof(uint16_t) * numOfIndices);

  return mesh;
}

void mesh_destroy(Mesh* mesh, VkDevice logicalDevice)
{
  gpu_buffer_free(&mesh->vertices, logicalDevice);
  gpu_buffer_free(&mesh->indices, logicalDevice);
  free(mesh);
}
