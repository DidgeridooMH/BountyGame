#include "Otter/Render/RayTracing/AccelerationStructure.h"

#include <stdlib.h>
#include <vulkan/vulkan_core.h>

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Util/Log.h"

static PFN_vkGetAccelerationStructureBuildSizesKHR
    _vkGetAccelerationStructureBuildSizesKHR;
static PFN_vkCreateAccelerationStructureKHR _vkCreateAccelerationStructureKHR;
static PFN_vkDestroyAccelerationStructureKHR _vkDestroyAccelerationStructureKHR;
static PFN_vkCmdBuildAccelerationStructuresKHR
    _vkCmdBuildAccelerationStructuresKHR;

#define LOAD_FUNCTION_EXTENSION(device, function)                        \
  _##function = (PFN_##function) vkGetDeviceProcAddr(device, #function); \
  if (function == NULL)                                                  \
  {                                                                      \
    LOG_ERROR("Failed to load " #function);                              \
    return false;                                                        \
  }

bool acceleration_structure_load_functions(VkDevice logicalDevice)
{
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkCreateAccelerationStructureKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkDestroyAccelerationStructureKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkCmdBuildAccelerationStructuresKHR);
  LOAD_FUNCTION_EXTENSION(
      logicalDevice, vkGetAccelerationStructureBuildSizesKHR);

  return true;
}

void acceleration_structure_create(AccelerationStructure* as)
{
  auto_array_create(&as->bottomLevel.accelerationStructures,
      sizeof(VkAccelerationStructureKHR));
  auto_array_create(&as->bottomLevel.asBuffers, sizeof(GpuBuffer));
  auto_array_create(
      &as->bottomLevel.geometries, sizeof(VkAccelerationStructureGeometryKHR));
  auto_array_create(&as->bottomLevel.geometryInfos,
      sizeof(VkAccelerationStructureBuildGeometryInfoKHR));
  auto_array_create(&as->bottomLevel.buildSizeInfos,
      sizeof(VkAccelerationStructureBuildSizesInfoKHR));
  auto_array_create(&as->bottomLevel.buildRangeInfos,
      sizeof(VkAccelerationStructureBuildRangeInfoKHR));
}

void acceleration_structure_destroy(
    AccelerationStructure* as, VkDevice logicalDevice)
{
  acceleration_structure_clear(as, logicalDevice);

  auto_array_destroy(&as->bottomLevel.accelerationStructures);
  auto_array_destroy(&as->bottomLevel.asBuffers);
  auto_array_destroy(&as->bottomLevel.geometries);
  auto_array_destroy(&as->bottomLevel.geometryInfos);
  auto_array_destroy(&as->bottomLevel.buildSizeInfos);
  auto_array_destroy(&as->bottomLevel.buildRangeInfos);
}

static void acceleration_structure_query_build_info(AutoArray* renderCommands,
    VkDeviceSize* totalAsSize, VkDeviceSize* maxScratchSize,
    AccelerationStructureLevel* asl, VkDevice logicalDevice)
{
  LOG_DEBUG("Allocating AS buffers");
  auto_array_allocate_many(&asl->accelerationStructures, renderCommands->size);
  auto_array_allocate_many(&asl->asBuffers, renderCommands->size);
  auto_array_allocate_many(&asl->geometries, renderCommands->size);
  auto_array_allocate_many(&asl->geometryInfos, renderCommands->size);
  auto_array_allocate_many(&asl->buildSizeInfos, renderCommands->size);
  auto_array_allocate_many(&asl->buildRangeInfos, renderCommands->size);

  for (size_t i = 0; i < renderCommands->size; i++)
  {
    RenderCommand* renderCommand = auto_array_get(renderCommands, i);
    VkAccelerationStructureGeometryKHR* geometry =
        auto_array_get(&asl->geometries, i);
    VkAccelerationStructureBuildGeometryInfoKHR* geometryInfo =
        auto_array_get(&asl->geometryInfos, i);
    VkAccelerationStructureBuildSizesInfoKHR* buildSizesInfo =
        auto_array_get(&asl->buildSizeInfos, i);
    VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo =
        auto_array_get(&asl->buildRangeInfos, i);

    GpuBuffer* vertices = &renderCommand->mesh->vertices;
    GpuBuffer* indices  = &renderCommand->mesh->indices;

    VkDeviceAddress vertexAddress =
        gpu_buffer_get_device_address(vertices, logicalDevice);
    VkDeviceAddress indexAddress =
        gpu_buffer_get_device_address(indices, logicalDevice);

    *geometry = (VkAccelerationStructureGeometryKHR){
        .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry.triangles =
            {
                .sType =
                    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT,
                .vertexData.deviceAddress = vertexAddress,
                .vertexStride             = sizeof(MeshVertex),
                .maxVertex = (vertices->size / sizeof(MeshVertex)) - 1,
                .indexType = VK_INDEX_TYPE_UINT16,
                .indexData.deviceAddress = indexAddress,
                // TODO: Add transformation
            },
        .flags = VK_GEOMETRY_OPAQUE_BIT_KHR};

    *buildRangeInfo = (VkAccelerationStructureBuildRangeInfoKHR){
        .primitiveCount  = (indices->size / sizeof(uint16_t)) / 3,
        .primitiveOffset = 0,
        .firstVertex     = 0,
        .transformOffset = 0,
    };

    *geometryInfo = (VkAccelerationStructureBuildGeometryInfoKHR){
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type  = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .geometryCount            = 1,
        .pGeometries              = geometry,
    };

    buildSizesInfo->sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    _vkGetAccelerationStructureBuildSizesKHR(logicalDevice,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, geometryInfo,
        &buildRangeInfo->primitiveCount, buildSizesInfo);

    *totalAsSize += buildSizesInfo->accelerationStructureSize;
    *maxScratchSize = max(buildSizesInfo->buildScratchSize, *maxScratchSize);
  }

  LOG_DEBUG("Total size: %llu", *totalAsSize);
  LOG_DEBUG("Scratchbuffer %llu", *maxScratchSize);
}

static bool acceleration_structure_create_blas(VkQueryPool queryPool,
    AccelerationStructureLevel* level, size_t startBatch, size_t endBatch,
    VkDeviceAddress scratchBuffer, VkCommandBuffer commandBuffer,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
{
#if 0
  vkResetQueryPool(logicalDevice, queryPool, 0, numOfCommands);
  size_t queryCount = 0;
#endif

  AutoArray buildRangeInfoPtrs;
  auto_array_create(
      &buildRangeInfoPtrs, sizeof(VkAccelerationStructureBuildRangeInfoKHR*));
  size_t scratchBufferOffset = 0;
  for (size_t i = startBatch; i < endBatch; i++)
  {
    GpuBuffer* asBuffer = auto_array_get(&level->asBuffers, i);
    VkAccelerationStructureBuildSizesInfoKHR* buildSizesInfo =
        auto_array_get(&level->buildSizeInfos, i);

    if (!gpu_buffer_allocate(asBuffer,
            buildSizesInfo->accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
                | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice))
    {
      LOG_ERROR("Failed to allocate buffer for acceleration structure");
      return false;
    }

    VkAccelerationStructureCreateInfoKHR createInfo = {
        .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .type   = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .size   = buildSizesInfo->accelerationStructureSize,
        .buffer = asBuffer->buffer,
    };

    VkAccelerationStructureKHR* accelerationStructure =
        auto_array_get(&level->accelerationStructures, i);
    if (_vkCreateAccelerationStructureKHR(
            logicalDevice, &createInfo, NULL, accelerationStructure)
        != VK_SUCCESS)
    {
      LOG_ERROR("Failed to create acceleration structure");
      return false;
    }

    VkAccelerationStructureBuildGeometryInfoKHR* geometryInfo =
        auto_array_get(&level->geometryInfos, i);
    geometryInfo->dstAccelerationStructure = *accelerationStructure;
    geometryInfo->scratchData.deviceAddress =
        scratchBuffer + scratchBufferOffset;
    scratchBufferOffset += buildSizesInfo->buildScratchSize;

    VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo =
        auto_array_get(&level->buildRangeInfos, i);
    VkAccelerationStructureBuildRangeInfoKHR** brip =
        auto_array_allocate(&buildRangeInfoPtrs);
    *brip = buildRangeInfo;
  }

  VkAccelerationStructureBuildGeometryInfoKHR* startingGeometryInfo =
      auto_array_get(&level->geometryInfos, startBatch);
  _vkCmdBuildAccelerationStructuresKHR(commandBuffer, buildRangeInfoPtrs.size,
      startingGeometryInfo, buildRangeInfoPtrs.buffer);

  VkMemoryBarrier memoryBarrier = {
      .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
      .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
  };
  vkCmdPipelineBarrier(commandBuffer,
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
      &memoryBarrier, 0, NULL, 0, NULL);

#if 0
    vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer, 1,
        &asUnit->accelerationStructure,
        VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool,
        queryCount++);
#endif

  return true;
}

// TODO: Allocate command buffers once and reuse them.
// TODO: Synchronize queries and building.
// TODO: Better cleanup on failure.
bool acceleration_structure_build(AccelerationStructure* accelerationStructure,
    AutoArray* renderCommands, VkCommandPool commandPool,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
{
  size_t totalAsSize    = 0;
  size_t maxScratchSize = 0;

  acceleration_structure_query_build_info(renderCommands, &totalAsSize,
      &maxScratchSize, &accelerationStructure->bottomLevel, logicalDevice);

  GpuBuffer scratchBuffer;
  if (!gpu_buffer_allocate(&scratchBuffer, 16 * maxScratchSize,
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
              | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice))
  {
    LOG_ERROR("Failed to allocate scratch buffer for acceleration structure");
    return false;
  }
  VkDeviceAddress scratchBufferAddress =
      gpu_buffer_get_device_address(&scratchBuffer, logicalDevice);

  VkQueryPoolCreateInfo queryPoolCreateInfo = {
      .sType      = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
      .queryType  = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR,
      .queryCount = renderCommands->size,
  };
  VkQueryPool queryPool;
  if (vkCreateQueryPool(logicalDevice, &queryPoolCreateInfo, NULL, &queryPool)
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to create query pool for acceleration structure");
    gpu_buffer_free(&scratchBuffer, logicalDevice);
    acceleration_structure_clear(accelerationStructure, logicalDevice);
    return false;
  }

  VkCommandBuffer blasCreateCommandBuffer;
  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool        = commandPool,
      .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  if (vkAllocateCommandBuffers(
          logicalDevice, &commandBufferAllocateInfo, &blasCreateCommandBuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to allocate command buffer for acceleration structure");
    gpu_buffer_free(&scratchBuffer, logicalDevice);
    return false;
  }

  VkQueue queue;
  vkGetDeviceQueue(logicalDevice, 0, 0, &queue);

  const VkDeviceSize BatchSizeLimit = 256 * 1024 * 1024;
  size_t batchByteSize              = 0;
  size_t batchSize                  = 0;
  const size_t asSize =
      accelerationStructure->bottomLevel.accelerationStructures.size;
  for (size_t i = 0; i < asSize; i++)
  {
    VkAccelerationStructureBuildSizesInfoKHR* buildSizesInfo =
        auto_array_get(&accelerationStructure->bottomLevel.buildSizeInfos, i);

    batchByteSize += buildSizesInfo->accelerationStructureSize;
    batchSize += 1;

    if (batchSize > 1 || batchByteSize >= BatchSizeLimit || i == asSize - 1)
    {
      LOG_DEBUG("Building batch [%llu, %llu)", i - batchSize + 1, i + 1);

      if (vkResetCommandBuffer(blasCreateCommandBuffer, 0) != VK_SUCCESS)
      {
        LOG_ERROR("Failed to create query pool for acceleration structure");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }
      if (vkBeginCommandBuffer(blasCreateCommandBuffer,
              &(VkCommandBufferBeginInfo){
                  .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                  .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
              })
          != VK_SUCCESS)
      {
        LOG_ERROR("Failed to begin command buffer");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }

      if (!acceleration_structure_create_blas(queryPool,
              &accelerationStructure->bottomLevel, i - batchSize + 1, i + 1,
              scratchBufferAddress, blasCreateCommandBuffer, logicalDevice,
              physicalDevice))
      {
        LOG_ERROR("Failed to create BLAS");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }

      if (vkEndCommandBuffer(blasCreateCommandBuffer) != VK_SUCCESS)
      {
        LOG_ERROR("Failed to end command buffer");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }

      LOG_DEBUG("Submitting build");

      VkSubmitInfo submitInfo = {
          .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          .commandBufferCount = 1,
          .pCommandBuffers    = &blasCreateCommandBuffer,
      };
      if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
      {
        LOG_ERROR("Failed to queue command buffer");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }
      // TODO: Prefer schronization.
      VkResult result = vkQueueWaitIdle(queue);
      if (result != VK_SUCCESS)
      {
        LOG_ERROR("Failed to wait on queue idle: %d", result);
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }

      batchSize     = 0;
      batchByteSize = 0;
    }
  }

  LOG_DEBUG("cleaning up build");

  gpu_buffer_free(&scratchBuffer, logicalDevice);
  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &blasCreateCommandBuffer);
  vkDestroyQueryPool(logicalDevice, queryPool, NULL);

  return true;
}

void acceleration_structure_clear(
    AccelerationStructure* as, VkDevice logicalDevice)
{
  for (size_t i = 0; i < as->bottomLevel.accelerationStructures.size; i++)
  {
    VkAccelerationStructureKHR* accelerationStructure =
        auto_array_get(&as->bottomLevel.accelerationStructures, i);
    _vkDestroyAccelerationStructureKHR(
        logicalDevice, *accelerationStructure, NULL);

    GpuBuffer* asBuffer = auto_array_get(&as->bottomLevel.asBuffers, i);
    gpu_buffer_free(asBuffer, logicalDevice);
  }

  auto_array_clear(&as->bottomLevel.accelerationStructures);
  auto_array_clear(&as->bottomLevel.asBuffers);
  auto_array_clear(&as->bottomLevel.geometries);
  auto_array_clear(&as->bottomLevel.geometryInfos);
  auto_array_clear(&as->bottomLevel.buildSizeInfos);
  auto_array_clear(&as->bottomLevel.buildRangeInfos);
}

