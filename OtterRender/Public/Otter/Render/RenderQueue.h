#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Mesh.h"
#include "Otter/Render/Uniform/Material.h"

typedef struct RenderCommand
{
  Mesh* mesh;
  Material material;
  Mat4 transform;
} RenderCommand;
