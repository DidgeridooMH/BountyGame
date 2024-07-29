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

  uint32_t missCount   = 1;
  uint32_t hitCount    = 1;
  uint32_t handleCount = 1 + missCount + hitCount;
  uint32_t handleSize  = shader_binding_table_round_aligned_size(
      rtProperties.shaderGroupHandleSize,
      rtProperties.shaderGroupHandleAlignment);

  sbt->rgenRegion = (VkStridedDeviceAddressRegionKHR){
      .stride = shader_binding_table_round_aligned_size(
          handleSize, rtProperties.shaderGroupBaseAlignment),
      .size = sbt->rgenRegion.stride,
  };
  sbt->missRegion = (VkStridedDeviceAddressRegionKHR){
      .stride = handleSize,
      .size   = shader_binding_table_round_aligned_size(
          missCount * handleSize, rtProperties.shaderGroupBaseAlignment),
  };
  sbt->hitRegion = (VkStridedDeviceAddressRegionKHR){
      .stride = handleSize,
      .size   = shader_binding_table_round_aligned_size(
          hitCount * handleSize, rtProperties.shaderGroupBaseAlignment),
  };

  uint32_t handlesSize = handleCount * rtProperties.shaderGroupHandleSize;
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
  if (!gpu_buffer_allocate(&sbt->sbt, handlesSize,
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
  memcpy(handleMapped, handles, rtProperties.shaderGroupHandleSize);
  handleMapped += sbt->rgenRegion.size;

  for (uint32_t i = 0; i < missCount; ++i)
  {
    memcpy(handleMapped, handles + (1 + i) * rtProperties.shaderGroupHandleSize,
        rtProperties.shaderGroupHandleSize);
    handleMapped += sbt->missRegion.stride;
  }

  for (uint32_t i = 0; i < hitCount; ++i)
  {
    memcpy(handleMapped,
        handles + (1 + missCount + i) * rtProperties.shaderGroupHandleSize,
        rtProperties.shaderGroupHandleSize);
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

