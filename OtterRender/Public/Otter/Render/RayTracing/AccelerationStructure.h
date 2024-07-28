#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/export.h"
#include "Otter/Util/AutoArray.h"

typedef struct AccelerationStructureLevel
{
  // @brief VkAccelerationStructureKHR
  AutoArray accelerationStructures;
  // @brief GpuBuffer
  AutoArray asBuffers;
  // @brief VkAccelerationStructureBuildGeometryKHR
  AutoArray geometries;
  // @brief VkAccelerationStructureBuildGeometryInfoKHR
  AutoArray geometryInfos;
  // @brief VkAccelerationStructureBuildSizesInfoKHR
  AutoArray buildSizeInfos;
  // @brief VkAccelerationStructureBuildRangeInfoKHR
  AutoArray buildRangeInfos;
} AccelerationStructureLevel;

typedef struct AccelerationStructure
{
  AccelerationStructureLevel bottomLevel;
} AccelerationStructure;

OTTERRENDER_API bool acceleration_structure_load_functions(
    VkDevice logicalDevice);

OTTERRENDER_API void acceleration_structure_create(AccelerationStructure* as);

OTTERRENDER_API void acceleration_structure_destroy(
    AccelerationStructure* as, VkDevice logicalDevice);

OTTERRENDER_API bool acceleration_structure_build(AccelerationStructure* as,
    AutoArray* renderCommands, VkCommandPool commandPool,
    VkDevice logicalDevice, VkPhysicalDevice physicalDevice);

OTTERRENDER_API void acceleration_structure_clear(
    AccelerationStructure* as, VkDevice logicalDevice);

