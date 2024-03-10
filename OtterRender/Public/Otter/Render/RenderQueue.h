#pragma once

#include "Otter/Util/Math/Transform.h"
#include <vulkan/vulkan.h>

typedef struct RenderCommand
{
  VkBuffer vertices;
  VkBuffer indices;
  VkDeviceSize numOfIndices;
  Transform transform;
} RenderCommand;
