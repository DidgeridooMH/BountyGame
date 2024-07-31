#include "Otter/Render/RayTracing/RayTracingFunctions.h"

PFN_vkGetAccelerationStructureBuildSizesKHR
    _vkGetAccelerationStructureBuildSizesKHR;
PFN_vkCreateAccelerationStructureKHR _vkCreateAccelerationStructureKHR;
PFN_vkDestroyAccelerationStructureKHR _vkDestroyAccelerationStructureKHR;
PFN_vkCmdBuildAccelerationStructuresKHR _vkCmdBuildAccelerationStructuresKHR;
PFN_vkCmdWriteAccelerationStructuresPropertiesKHR
    _vkCmdWriteAccelerationStructuresPropertiesKHR;
PFN_vkGetRayTracingShaderGroupHandlesKHR _vkGetRayTracingShaderGroupHandlesKHR;
PFN_vkCreateRayTracingPipelinesKHR _vkCreateRayTracingPipelinesKHR;
PFN_vkCmdTraceRaysKHR _vkCmdTraceRaysKHR;
PFN_vkGetAccelerationStructureDeviceAddressKHR
    _vkGetAccelerationStructureDeviceAddressKHR;

bool ray_tracing_load_functions(VkDevice logicalDevice)
{
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkCreateAccelerationStructureKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkDestroyAccelerationStructureKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkCmdBuildAccelerationStructuresKHR);
  LOAD_FUNCTION_EXTENSION(
      logicalDevice, vkGetAccelerationStructureBuildSizesKHR);
  LOAD_FUNCTION_EXTENSION(
      logicalDevice, vkCmdWriteAccelerationStructuresPropertiesKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkGetRayTracingShaderGroupHandlesKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkCreateRayTracingPipelinesKHR);
  LOAD_FUNCTION_EXTENSION(logicalDevice, vkCmdTraceRaysKHR);
  LOAD_FUNCTION_EXTENSION(
      logicalDevice, vkGetAccelerationStructureDeviceAddressKHR);

  return true;
}

