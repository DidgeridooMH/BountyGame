#pragma once

#include "Otter/Math/Transform.h"
#include <vulkan/vulkan.h>

typedef struct RenderCommand
{
  VkBuffer vertices;
  VkBuffer indices;
  VkDeviceSize numOfIndices;
  MeshVertex* cpuVertices;
  size_t numOfVertices;
  uint16_t* cpuIndices;
  Transform transform;
} RenderCommand;
