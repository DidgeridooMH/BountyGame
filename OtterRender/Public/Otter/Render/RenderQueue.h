#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Mesh.h"

typedef struct RenderCommand
{
  VkBuffer vertices;
  VkBuffer indices;
  VkDeviceSize numOfIndices;
  MeshVertex* cpuVertices;
  size_t numOfVertices;
  uint16_t* cpuIndices;
  Mat4 transform;
} RenderCommand;
