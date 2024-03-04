#pragma once

#include <vulkan/vulkan.h>

typedef struct RenderCommand
{
  VkBuffer vertices;
  VkBuffer indices;
  VkDeviceSize numOfIndices;
  VkPipeline pipeline;
  // TODO: Add transform.
} RenderCommand;
