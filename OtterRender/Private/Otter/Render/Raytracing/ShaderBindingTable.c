#include "Otter/Render/RayTracing/ShaderBindingTable.h"

#include "Otter/Render/RayTracing/RayTracingFunctions.h"

uint32_t shader_binding_table_round_aligned_size(
    uint32_t value, uint32_t alignment)
{
  return (value + alignment - 1) & ~(alignment - 1);
}

bool shader_binding_table_create(ShaderBindingTable* sbt,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkPipeline pipeline)
{
  VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProperties = {
      .sType =
          VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
  };
  VkPhysicalDeviceProperties2 properties = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &rtProperties,
  };
  vkGetPhysicalDeviceProperties2(physicalDevice, &properties);

  const uint32_t raygenCount       = 1;
  const uint32_t missCount         = 1;
  const uint32_t hitCount          = 1;
  const uint32_t handleCount       = raygenCount + missCount + hitCount;
  const uint32_t handleSize        = rtProperties.shaderGroupHandleSize;
  const uint32_t alignedHandleSize = shader_binding_table_round_aligned_size(
      handleSize, rtProperties.shaderGroupHandleAlignment);

  const uint32_t alignedBaseSize = shader_binding_table_round_aligned_size(
      alignedHandleSize, rtProperties.shaderGroupBaseAlignment);
  sbt->rgenRegion.stride = alignedBaseSize;
  sbt->rgenRegion.size   = sbt->rgenRegion.stride;

  sbt->missRegion = (VkStridedDeviceAddressRegionKHR){
      .stride = alignedHandleSize,
      .size   = shader_binding_table_round_aligned_size(
          missCount * alignedHandleSize, rtProperties.shaderGroupBaseAlignment),
  };

  sbt->hitRegion = (VkStridedDeviceAddressRegionKHR){
      .stride = alignedHandleSize,
      .size   = shader_binding_table_round_aligned_size(
          hitCount * alignedHandleSize, rtProperties.shaderGroupBaseAlignment),
  };

  memset(&sbt->callRegion, 0, sizeof(VkStridedDeviceAddressRegionKHR));

  uint32_t handlesSize = handleCount * handleSize;
  uint8_t* handles     = malloc(handlesSize);
  if (handles == NULL)
  {
    return false;
  }

  if (_vkGetRayTracingShaderGroupHandlesKHR(
          logicalDevice, pipeline, 0, handleCount, handlesSize, handles)
      != VK_SUCCESS)
  {
    free(handles);
    return false;
  }

  VkDeviceSize sbtSize =
      sbt->rgenRegion.size + sbt->missRegion.size + sbt->hitRegion.size;
  if (!gpu_buffer_allocate(&sbt->sbt, sbtSize,
          VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
              | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          physicalDevice, logicalDevice))
  {
    free(handles);
    return false;
  }

  VkDeviceAddress sbtAddress =
      gpu_buffer_get_device_address(&sbt->sbt, logicalDevice);
  sbt->rgenRegion.deviceAddress = sbtAddress;
  sbt->missRegion.deviceAddress = sbtAddress + sbt->rgenRegion.size;
  sbt->hitRegion.deviceAddress =
      sbt->missRegion.deviceAddress + sbt->missRegion.size;

  gpu_buffer_map_all(&sbt->sbt, logicalDevice);
  uint8_t* handleMapped = sbt->sbt.mapped;
  memcpy(handleMapped, handles, handleSize);
  handleMapped += sbt->rgenRegion.size;

  uint32_t handleIndex = 1;

  for (uint32_t i = 0; i < missCount; ++i)
  {
    memcpy(handleMapped, handles + (handleIndex++ * handleSize), handleSize);
    handleMapped += sbt->missRegion.stride;
  }
  handleMapped =
      (uint8_t*) sbt->sbt.mapped + sbt->rgenRegion.size + sbt->missRegion.size;

  for (uint32_t i = 0; i < hitCount; ++i)
  {
    memcpy(handleMapped, handles + (handleIndex++ * handleSize), handleSize);
    handleMapped += sbt->hitRegion.stride;
  }

  gpu_buffer_unmap(&sbt->sbt, logicalDevice);

  free(handles);

  return true;
}

void shader_binding_table_destroy(
    ShaderBindingTable* sbt, VkDevice logicalDevice)
{
  gpu_buffer_free(&sbt->sbt, logicalDevice);
}
