#pragma once

#include "Otter/Util/Math/Mat.h"
#include <vulkan/vulkan.h>

typedef struct RenderCommand
{
  VkBuffer vertices;
  VkBuffer indices;
  VkDeviceSize numOfIndices;
  Mat4 transform;
} RenderCommand;
