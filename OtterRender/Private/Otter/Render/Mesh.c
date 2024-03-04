#include "Otter/Render/Mesh.h"

Mesh* mesh_create(const MeshVertex vertices[], uint64_t numOfVertices,
    const uint16_t indices[], uint64_t numOfIndices,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandPool commandPool, VkQueue commandQueue)
{
  Mesh* mesh = malloc(sizeof(Mesh));
  if (mesh == NULL)
  {
    return NULL;
  }

  mesh->vertices = gpu_buffer_allocate(numOfVertices * sizeof(vertices[0]),
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice);

  mesh->indices = gpu_buffer_allocate(numOfIndices * sizeof(indices[0]),
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice);

  GpuBuffer* vertexStagingBuffer = gpu_buffer_allocate(mesh->vertices->size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      physicalDevice, logicalDevice);

  GpuBuffer* indexStagingBuffer =
      gpu_buffer_allocate(mesh->indices->size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice);

  if (mesh->vertices == NULL || mesh->indices == NULL
      || vertexStagingBuffer == NULL || indexStagingBuffer == NULL)
  {
    fprintf(stderr, "There was a problem allocating the mesh\n");
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
    return NULL;
  }

  if (!gpu_buffer_write(vertexStagingBuffer, (uint8_t*) vertices,
          mesh->vertices->size, 0, logicalDevice)
      || !gpu_buffer_write(indexStagingBuffer, (uint8_t*) indices,
          mesh->indices->size, 0, logicalDevice))
  {
    fprintf(stderr, "There was an issue writing to the buffer.\n");
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
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
    fprintf(stderr, "Unable to allocate command buffer\n");
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
    return NULL;
  }

  VkCommandBufferBeginInfo beginInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
  if (vkBeginCommandBuffer(transferCommands, &beginInfo) != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to start command buffer\n");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
    return NULL;
  }

  gpu_buffer_transfer(vertexStagingBuffer, mesh->vertices, transferCommands);
  gpu_buffer_transfer(indexStagingBuffer, mesh->indices, transferCommands);

  if (vkEndCommandBuffer(transferCommands) != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to end command buffer\n");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
    return NULL;
  }

  VkSubmitInfo submitInfo = {.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pCommandBuffers              = &transferCommands,
      .commandBufferCount           = 1};
  if (vkQueueSubmit(commandQueue, 1, &submitInfo, NULL) != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to submit queue\n");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
    return NULL;
  }

  if (vkQueueWaitIdle(commandQueue) != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to start command buffer\n");
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);
    gpu_buffer_free(mesh->vertices, logicalDevice);
    gpu_buffer_free(mesh->indices, logicalDevice);
    gpu_buffer_free(vertexStagingBuffer, logicalDevice);
    gpu_buffer_free(indexStagingBuffer, logicalDevice);
    free(mesh);
    return NULL;
  }

  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &transferCommands);

  gpu_buffer_free(vertexStagingBuffer, logicalDevice);
  gpu_buffer_free(indexStagingBuffer, logicalDevice);

  return mesh;
}

void mesh_destroy(Mesh* mesh, VkDevice logicalDevice)
{
  gpu_buffer_free(mesh->vertices, logicalDevice);
  gpu_buffer_free(mesh->indices, logicalDevice);
  free(mesh);
}
