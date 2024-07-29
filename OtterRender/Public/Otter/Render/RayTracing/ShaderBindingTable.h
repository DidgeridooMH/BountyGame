#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Memory/GpuBuffer.h"

typedef struct ShaderBindingTable
{
  VkStridedDeviceAddressRegionKHR rgenRegion;
  VkStridedDeviceAddressRegionKHR missRegion;
  VkStridedDeviceAddressRegionKHR hitRegion;
  GpuBuffer sbt;
} ShaderBindingTable;

bool shader_binding_table_create(ShaderBindingTable* sbt,
    VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkPipeline pipeline);

void shader_binding_table_destroy(
    ShaderBindingTable* sbt, VkDevice logicalDevice);

