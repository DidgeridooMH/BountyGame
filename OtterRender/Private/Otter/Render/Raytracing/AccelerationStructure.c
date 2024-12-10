#include "Otter/Render/RayTracing/AccelerationStructure.h"

#include <stdlib.h>

#include "Otter/Render/Memory/GpuBuffer.h"
#include "Otter/Render/MeshVertex.h"
#include "Otter/Render/RayTracing/RayTracingFunctions.h"
#include "Otter/Render/RenderQueue.h"
#include "Otter/Util/Log.h"

static void acceleration_structure_create_level(
    AccelerationStructureLevel* asLevel)
{
  auto_array_create(
      &asLevel->accelerationStructures, sizeof(VkAccelerationStructureKHR));
  auto_array_create(&asLevel->asBuffers, sizeof(GpuBuffer));
  auto_array_create(
      &asLevel->geometries, sizeof(VkAccelerationStructureGeometryKHR));
  auto_array_create(&asLevel->geometryInfos,
      sizeof(VkAccelerationStructureBuildGeometryInfoKHR));
  auto_array_create(&asLevel->buildSizeInfos,
      sizeof(VkAccelerationStructureBuildSizesInfoKHR));
  auto_array_create(&asLevel->buildRangeInfos,
      sizeof(VkAccelerationStructureBuildRangeInfoKHR));
}

void acceleration_structure_create(AccelerationStructure* as)
{
  acceleration_structure_create_level(&as->bottomLevel);
  acceleration_structure_create_level(&as->topLevel);
}

static void acceleration_structure_destroy_level(
    AccelerationStructureLevel* as, VkDevice logicalDevice)
{
  for (size_t i = 0; i < as->accelerationStructures.size; i++)
  {
    VkAccelerationStructureKHR* accelerationStructure =
        auto_array_get(&as->accelerationStructures, i);
    _vkDestroyAccelerationStructureKHR(
        logicalDevice, *accelerationStructure, NULL);
  }

  for (size_t i = 0; i < as->asBuffers.size; i++)
  {
    GpuBuffer* asBuffer = auto_array_get(&as->asBuffers, i);
    gpu_buffer_free(asBuffer, logicalDevice);
  }

  auto_array_destroy(&as->accelerationStructures);
  auto_array_destroy(&as->asBuffers);
  auto_array_destroy(&as->geometries);
  auto_array_destroy(&as->geometryInfos);
  auto_array_destroy(&as->buildSizeInfos);
  auto_array_destroy(&as->buildRangeInfos);
}

void acceleration_structure_destroy(
    AccelerationStructure* as, VkDevice logicalDevice)
{
  acceleration_structure_destroy_level(&as->bottomLevel, logicalDevice);
  acceleration_structure_destroy_level(&as->topLevel, logicalDevice);

  gpu_buffer_free(&as->bottomLevelInstances, logicalDevice);
}

static void acceleration_structure_query_build_info(VkDeviceSize* totalAsSize,
    VkDeviceSize* maxScratchSize, AccelerationStructureLevel* asl,
    VkAccelerationStructureTypeKHR levelType, VkDevice logicalDevice)
{
  for (size_t i = 0; i < asl->geometries.size; i++)
  {
    VkAccelerationStructureGeometryKHR* geometry =
        auto_array_get(&asl->geometries, i);
    VkAccelerationStructureBuildGeometryInfoKHR* geometryInfo =
        auto_array_get(&asl->geometryInfos, i);
    VkAccelerationStructureBuildSizesInfoKHR* buildSizesInfo =
        auto_array_get(&asl->buildSizeInfos, i);
    VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo =
        auto_array_get(&asl->buildRangeInfos, i);

    *geometryInfo = (VkAccelerationStructureBuildGeometryInfoKHR){
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type  = levelType,
        .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .srcAccelerationStructure = VK_NULL_HANDLE,
        .dstAccelerationStructure = VK_NULL_HANDLE,
        .geometryCount            = 1,
        .pGeometries              = geometry,
    };

    if (levelType == VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
    {
      geometryInfo->flags |=
          VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
    }

    buildSizesInfo->sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    _vkGetAccelerationStructureBuildSizesKHR(logicalDevice,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, geometryInfo,
        &buildRangeInfo->primitiveCount, buildSizesInfo);

    *totalAsSize += buildSizesInfo->accelerationStructureSize;
    *maxScratchSize = max(buildSizesInfo->buildScratchSize, *maxScratchSize);
  }
}

static bool acceleration_structure_create_blas(VkQueryPool queryPool,
    AccelerationStructureLevel* level, size_t startBatch, size_t endBatch,
    VkDeviceAddress scratchBuffer, VkCommandBuffer commandBuffer,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkQueue queue)
{
  if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to create query pool for acceleration structure");
    return false;
  }
  if (vkBeginCommandBuffer(commandBuffer,
          &(VkCommandBufferBeginInfo){
              .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          })
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to begin command buffer");
    return false;
  }

  vkResetQueryPool(logicalDevice, queryPool, startBatch, endBatch - startBatch);

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

  /*_vkCmdWriteAccelerationStructuresPropertiesKHR(commandBuffer,
      endBatch - startBatch,
      auto_array_get(&level->accelerationStructures, startBatch),
      VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, queryPool,
      startBatch);*/

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to end command buffer");
    return false;
  }

  VkSubmitInfo submitInfo = {
      .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers    = &commandBuffer,
  };
  if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to queue command buffer");
    return false;
  }
  // TODO: Prefer schronization.
  VkResult result = vkQueueWaitIdle(queue);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Failed to wait on queue idle: %d", result);
    return false;
  }

  return true;
}

static bool acceleration_structure_compact_blas(VkQueryPool queryPool,
    AccelerationStructureLevel* level, size_t startBatch, size_t endBatch,
    VkDeviceAddress scratchBuffer, VkCommandBuffer commandBuffer,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VkQueue queue)
{
  if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to create query pool for acceleration structure");
    return false;
  }
  if (vkBeginCommandBuffer(commandBuffer,
          &(VkCommandBufferBeginInfo){
              .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          })
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to begin command buffer");
    return false;
  }

  AutoArray compactedSizes;
  auto_array_create(&compactedSizes, sizeof(VkDeviceSize));
  auto_array_allocate_many(&compactedSizes, endBatch - startBatch);

  AutoArray oldAsBuffers;
  auto_array_create(&oldAsBuffers, sizeof(GpuBuffer));
  auto_array_allocate_many(&oldAsBuffers, endBatch - startBatch);

  AutoArray oldAccelerationStructures;
  auto_array_create(
      &oldAccelerationStructures, sizeof(VkAccelerationStructureKHR));
  auto_array_allocate_many(&oldAccelerationStructures, endBatch - startBatch);

  vkGetQueryPoolResults(logicalDevice, queryPool, startBatch,
      compactedSizes.size, compactedSizes.size * sizeof(VkDeviceSize),
      compactedSizes.buffer, sizeof(VkDeviceSize),
      VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

  for (size_t i = startBatch; i < endBatch; i++)
  {
    VkDeviceSize* compactedSize =
        auto_array_get(&compactedSizes, i - startBatch);
    if (*compactedSize == 0)
    {
      continue;
    }

    GpuBuffer* asBuffer = auto_array_get(&level->asBuffers, i);
    VkAccelerationStructureKHR* accelerationStructure =
        auto_array_get(&level->accelerationStructures, i);

    GpuBuffer* oldAsBuffer = auto_array_get(&oldAsBuffers, i - startBatch);
    *oldAsBuffer           = *asBuffer;
    VkAccelerationStructureKHR* oldAccelerationStructure =
        auto_array_get(&oldAccelerationStructures, i - startBatch);
    *oldAccelerationStructure = *accelerationStructure;

    if (!gpu_buffer_allocate(asBuffer, *compactedSize,
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
        .size   = *compactedSize,
        .buffer = asBuffer->buffer,
    };

    if (_vkCreateAccelerationStructureKHR(
            logicalDevice, &createInfo, NULL, accelerationStructure)
        != VK_SUCCESS)
    {
      LOG_ERROR("Failed to create acceleration structure");
      return false;
    }
  }

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to end command buffer");
    return false;
  }

  VkSubmitInfo submitInfo = (VkSubmitInfo){
      .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers    = &commandBuffer,
  };
  if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to queue command buffer");
    return false;
  }

  VkResult result = vkQueueWaitIdle(queue);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Failed to wait on queue idle: %d", result);
    return false;
  }

  for (size_t i = startBatch; i < endBatch; i++)
  {
    GpuBuffer* oldAsBuffer = auto_array_get(&oldAsBuffers, i - startBatch);
    gpu_buffer_free(oldAsBuffer, logicalDevice);

    VkAccelerationStructureKHR* oldAccelerationStructure =
        auto_array_get(&oldAccelerationStructures, i - startBatch);
    _vkDestroyAccelerationStructureKHR(
        logicalDevice, *oldAccelerationStructure, NULL);
  }

  auto_array_destroy(&compactedSizes);
  auto_array_destroy(&oldAsBuffers);
  auto_array_destroy(&oldAccelerationStructures);

  return true;
}

static bool acceleration_structure_build_blas(AccelerationStructure* as,
    AutoArray* renderCommands, VkCommandBuffer commandBuffer,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
{
  const size_t ScratchBufferMultiplier = 4;
  size_t totalAsSize                   = 0;
  size_t maxScratchSize                = 0;

  auto_array_allocate_many(
      &as->bottomLevel.accelerationStructures, renderCommands->size);
  auto_array_allocate_many(&as->bottomLevel.asBuffers, renderCommands->size);
  auto_array_allocate_many(&as->bottomLevel.geometries, renderCommands->size);
  auto_array_allocate_many(
      &as->bottomLevel.geometryInfos, renderCommands->size);
  auto_array_allocate_many(
      &as->bottomLevel.buildSizeInfos, renderCommands->size);
  auto_array_allocate_many(
      &as->bottomLevel.buildRangeInfos, renderCommands->size);

  for (size_t i = 0; i < as->bottomLevel.geometries.size; i++)
  {
    VkAccelerationStructureGeometryKHR* geometry =
        auto_array_get(&as->bottomLevel.geometries, i);
    RenderCommand* renderCommand = auto_array_get(renderCommands, i);

    GpuBuffer* vertices = &renderCommand->mesh->vertices;
    GpuBuffer* indices  = &renderCommand->mesh->indices;

    VkDeviceAddress vertexAddress =
        gpu_buffer_get_device_address(vertices, logicalDevice);
    VkDeviceAddress indexAddress =
        gpu_buffer_get_device_address(indices, logicalDevice);

    *geometry = (VkAccelerationStructureGeometryKHR){
        .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry.triangles = {
            .sType =
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
            .vertexFormat             = VK_FORMAT_R32G32B32_SFLOAT,
            .vertexData.deviceAddress = vertexAddress,
            .vertexStride             = sizeof(MeshVertex),
            .maxVertex = (vertices->size / sizeof(MeshVertex)) - 1,
            .indexType = VK_INDEX_TYPE_UINT16,
            .indexData.deviceAddress = indexAddress,
        }};

    VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo =
        auto_array_get(&as->bottomLevel.buildRangeInfos, i);
    *buildRangeInfo = (VkAccelerationStructureBuildRangeInfoKHR){
        .primitiveCount  = (indices->size / sizeof(uint16_t)) / 3,
        .primitiveOffset = 0,
        .firstVertex     = 0,
        .transformOffset = 0,
    };
  }

  acceleration_structure_query_build_info(&totalAsSize, &maxScratchSize,
      &as->bottomLevel, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
      logicalDevice);

  GpuBuffer scratchBuffer;
  if (!gpu_buffer_allocate(&scratchBuffer,
          ScratchBufferMultiplier * maxScratchSize,
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
    acceleration_structure_clear(as, logicalDevice);
    return false;
  }

  VkQueue queue;
  vkGetDeviceQueue(logicalDevice, 0, 0, &queue);

  const VkDeviceSize BatchSizeLimit = 256 * 1024 * 1024;
  size_t batchByteSize              = 0;
  size_t batchScratchSize           = 0;
  size_t batchStart                 = 0;
  const size_t asSize = as->bottomLevel.accelerationStructures.size;
  for (size_t i = 0; i <= asSize; i++)
  {
    if (i < asSize)
    {
      VkAccelerationStructureBuildSizesInfoKHR* buildSizesInfo =
          auto_array_get(&as->bottomLevel.buildSizeInfos, i);

      batchByteSize += buildSizesInfo->accelerationStructureSize;
      batchScratchSize += buildSizesInfo->buildScratchSize;
    }

    if (batchScratchSize >= scratchBuffer.size
        || batchByteSize >= BatchSizeLimit || i == asSize)
    {
      if (!acceleration_structure_create_blas(queryPool, &as->bottomLevel,
              batchStart, i, scratchBufferAddress, commandBuffer, logicalDevice,
              physicalDevice, queue))
      {
        LOG_ERROR("Failed to create BLAS");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }

      /*if (!acceleration_structure_compact_blas(queryPool, &as->bottomLevel,
              batchStart, i, scratchBufferAddress, commandBuffer, logicalDevice,
              physicalDevice, queue))
      {
        LOG_ERROR("Failed to compact BLAS");
        gpu_buffer_free(&scratchBuffer, logicalDevice);
        return false;
      }*/

      batchStart = i;

      if (i < asSize)
      {
        VkAccelerationStructureBuildSizesInfoKHR* buildSizesInfo =
            auto_array_get(&as->bottomLevel.buildSizeInfos, i);
        batchByteSize    = buildSizesInfo->accelerationStructureSize;
        batchScratchSize = buildSizesInfo->buildScratchSize;
      }
    }
  }

  gpu_buffer_free(&scratchBuffer, logicalDevice);
  vkDestroyQueryPool(logicalDevice, queryPool, NULL);

  return true;
}

static bool acceleration_structure_build_tlas(AccelerationStructure* as,
    RenderCommand* renderCommands, VkCommandBuffer commandBuffer,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to create query pool for acceleration structure");
    return false;
  }
  if (vkBeginCommandBuffer(commandBuffer,
          &(VkCommandBufferBeginInfo){
              .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
              .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
          })
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to begin command buffer");
    return false;
  }

  AutoArray asInstances;
  auto_array_create(&asInstances, sizeof(VkAccelerationStructureInstanceKHR));
  auto_array_allocate_many(
      &asInstances, as->bottomLevel.accelerationStructures.size);

  for (size_t i = 0; i < asInstances.size; i++)
  {
    VkAccelerationStructureInstanceKHR* instance =
        auto_array_get(&asInstances, i);
    VkAccelerationStructureDeviceAddressInfoKHR asDeviceAddress = {
        .sType =
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .accelerationStructure = *(VkAccelerationStructureKHR*) auto_array_get(
            &as->bottomLevel.accelerationStructures, i),
    };
    *instance = (VkAccelerationStructureInstanceKHR){
        .transform =
            {
                .matrix =
                    {
                        {
                            renderCommands[i].transform[0][0],
                            renderCommands[i].transform[1][0],
                            renderCommands[i].transform[2][0],
                            renderCommands[i].transform[3][0],
                        },
                        {
                            renderCommands[i].transform[0][1],
                            renderCommands[i].transform[1][1],
                            renderCommands[i].transform[2][1],
                            renderCommands[i].transform[3][1],
                        },
                        {
                            renderCommands[i].transform[0][2],
                            renderCommands[i].transform[1][2],
                            renderCommands[i].transform[2][2],
                            renderCommands[i].transform[3][2],
                        },
                    },
            },
        .instanceCustomIndex                    = 0,
        .mask                                   = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference =
            _vkGetAccelerationStructureDeviceAddressKHR(
                logicalDevice, &asDeviceAddress),
    };
  }

  GpuBuffer instancesStagingBuffer;
  if (!gpu_buffer_allocate(&instancesStagingBuffer,
          asInstances.size * sizeof(VkAccelerationStructureInstanceKHR),
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    LOG_ERROR("Failed to allocate instances staging buffer for acceleration "
              "structure");
    return false;
  }

  if (!gpu_buffer_write(&instancesStagingBuffer, asInstances.buffer,
          asInstances.size * sizeof(VkAccelerationStructureInstanceKHR), 0,
          logicalDevice))
  {
    LOG_ERROR("Failed to write instances buffer for acceleration structure");
    gpu_buffer_free(&instancesStagingBuffer, logicalDevice);
    return false;
  }

  if (!gpu_buffer_allocate(&as->bottomLevelInstances,
          instancesStagingBuffer.size,
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
              | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
              | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice))
  {
    LOG_ERROR("Failed to allocate instances buffer for acceleration structure");
    return false;
  }

  gpu_buffer_transfer(
      &instancesStagingBuffer, &as->bottomLevelInstances, commandBuffer);

  VkMemoryBarrier memoryBarrier = {
      .sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
  };
  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1,
      &memoryBarrier, 0, NULL, 0, NULL);

  VkAccelerationStructureGeometryKHR* geometry =
      auto_array_allocate(&as->topLevel.geometries);
  *geometry = (VkAccelerationStructureGeometryKHR){
      .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
      .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
      .geometry.instances =
          {
              .sType =
                  VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
              .arrayOfPointers    = VK_FALSE,
              .data.deviceAddress = gpu_buffer_get_device_address(
                  &as->bottomLevelInstances, logicalDevice),
          },
  };

  VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfo =
      auto_array_allocate(&as->topLevel.buildRangeInfos);
  *buildRangeInfo = (VkAccelerationStructureBuildRangeInfoKHR){
      .primitiveCount  = asInstances.size,
      .primitiveOffset = 0,
      .firstVertex     = 0,
      .transformOffset = 0,
  };

  VkAccelerationStructureKHR* accelerationStructure =
      auto_array_allocate(&as->topLevel.accelerationStructures);
  GpuBuffer* asBuffer = auto_array_allocate(&as->topLevel.asBuffers);
  VkAccelerationStructureBuildGeometryInfoKHR* geometryInfo =
      auto_array_allocate(&as->topLevel.geometryInfos);
  VkAccelerationStructureBuildSizesInfoKHR* buildSizeInfo =
      auto_array_allocate(&as->topLevel.buildSizeInfos);

  *geometryInfo = (VkAccelerationStructureBuildGeometryInfoKHR){
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type  = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .mode  = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .geometryCount = as->topLevel.geometries.size,
      .pGeometries   = geometry,
  };

  buildSizeInfo->sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  _vkGetAccelerationStructureBuildSizesKHR(logicalDevice,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, geometryInfo,
      &buildRangeInfo->primitiveCount, buildSizeInfo);

  VkDeviceSize totalAsSize    = buildSizeInfo->accelerationStructureSize;
  VkDeviceSize maxScratchSize = buildSizeInfo->buildScratchSize;

  GpuBuffer scratchBuffer;
  if (!gpu_buffer_allocate(&scratchBuffer, maxScratchSize,
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
              | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice))
  {
    LOG_ERROR("Failed to allocate scratch buffer for acceleration structure");
    return false;
  }
  VkDeviceAddress scratchBufferAddress =
      gpu_buffer_get_device_address(&scratchBuffer, logicalDevice);

  if (!gpu_buffer_allocate(asBuffer, totalAsSize,
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR
              | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice))
  {
    LOG_ERROR("Failed to allocate buffer for acceleration structure");
    return false;
  }

  VkAccelerationStructureCreateInfoKHR createInfo = {
      .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
      .type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
      .size   = totalAsSize,
      .buffer = asBuffer->buffer,
  };
  if (_vkCreateAccelerationStructureKHR(
          logicalDevice, &createInfo, NULL, accelerationStructure)
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to create acceleration structure");
    gpu_buffer_free(&scratchBuffer, logicalDevice);
    return false;
  }

  geometryInfo->dstAccelerationStructure  = *accelerationStructure;
  geometryInfo->scratchData.deviceAddress = scratchBufferAddress;

  const VkAccelerationStructureBuildRangeInfoKHR* buildRangeInfoPtr =
      buildRangeInfo;
  _vkCmdBuildAccelerationStructuresKHR(
      commandBuffer, 1, geometryInfo, &buildRangeInfoPtr);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to end command buffer");
    return false;
  }

  VkQueue queue;
  vkGetDeviceQueue(logicalDevice, 0, 0, &queue);

  VkSubmitInfo submitInfo = {
      .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers    = &commandBuffer,
  };
  if (vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
  {
    LOG_ERROR("Failed to queue command buffer");
    return false;
  }
  // TODO: Prefer schronization.
  VkResult result = vkQueueWaitIdle(queue);
  if (result != VK_SUCCESS)
  {
    LOG_ERROR("Failed to wait on queue idle: %d", result);
    return false;
  }

  gpu_buffer_free(&scratchBuffer, logicalDevice);
  gpu_buffer_free(&instancesStagingBuffer, logicalDevice);
  auto_array_destroy(&asInstances);

  return true;
}

// TODO: Allocate command buffers once and reuse them.
// TODO: Synchronize queries and building.
// TODO: Better cleanup on failure.
bool acceleration_structure_build(AccelerationStructure* as,
    AutoArray* renderCommands, VkCommandPool commandPool,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice)
{
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
    return false;
  }

  if (!acceleration_structure_build_blas(as, renderCommands,
          blasCreateCommandBuffer, logicalDevice, physicalDevice))
  {
    LOG_ERROR("Failed to build BLAS");
    return false;
  }

  if (!acceleration_structure_build_tlas(as, renderCommands->buffer,
          blasCreateCommandBuffer, physicalDevice, logicalDevice))
  {
    LOG_ERROR("Failed to build TLAS");
    return false;
  }

  vkFreeCommandBuffers(logicalDevice, commandPool, 1, &blasCreateCommandBuffer);

  return true;
}

static void acceleration_structure_clear_level(
    AccelerationStructureLevel* as, VkDevice logicalDevice)
{
  for (size_t i = 0; i < as->accelerationStructures.size; i++)
  {
    VkAccelerationStructureKHR* accelerationStructure =
        auto_array_get(&as->accelerationStructures, i);
    _vkDestroyAccelerationStructureKHR(
        logicalDevice, *accelerationStructure, NULL);

    GpuBuffer* asBuffer = auto_array_get(&as->asBuffers, i);
    gpu_buffer_free(asBuffer, logicalDevice);
  }

  auto_array_clear(&as->accelerationStructures);
  auto_array_clear(&as->asBuffers);
  auto_array_clear(&as->geometries);
  auto_array_clear(&as->geometryInfos);
  auto_array_clear(&as->buildSizeInfos);
  auto_array_clear(&as->buildRangeInfos);
}

void acceleration_structure_clear(
    AccelerationStructure* as, VkDevice logicalDevice)
{
  acceleration_structure_clear_level(&as->bottomLevel, logicalDevice);
  acceleration_structure_clear_level(&as->topLevel, logicalDevice);
}
