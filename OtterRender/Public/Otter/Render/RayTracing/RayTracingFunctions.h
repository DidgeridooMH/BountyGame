#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Util/Log.h"

#define LOAD_FUNCTION_EXTENSION(device, function)                        \
  _##function = (PFN_##function) vkGetDeviceProcAddr(device, #function); \
  if (function == NULL)                                                  \
  {                                                                      \
    LOG_ERROR("Failed to load " #function);                              \
    return false;                                                        \
  }

bool ray_tracing_load_functions(VkDevice logicalDevice);

extern PFN_vkGetAccelerationStructureBuildSizesKHR
    _vkGetAccelerationStructureBuildSizesKHR;
extern PFN_vkCreateAccelerationStructureKHR _vkCreateAccelerationStructureKHR;
extern PFN_vkDestroyAccelerationStructureKHR _vkDestroyAccelerationStructureKHR;
extern PFN_vkCmdBuildAccelerationStructuresKHR
    _vkCmdBuildAccelerationStructuresKHR;
extern PFN_vkCmdWriteAccelerationStructuresPropertiesKHR
    _vkCmdWriteAccelerationStructuresPropertiesKHR;
extern PFN_vkGetRayTracingShaderGroupHandlesKHR
    _vkGetRayTracingShaderGroupHandlesKHR;
extern PFN_vkCreateRayTracingPipelinesKHR _vkCreateRayTracingPipelinesKHR;

