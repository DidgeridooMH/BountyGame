#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Mesh.h"
#include "Otter/Render/Uniform/Material.h"
#include "Otter/Render/export.h"

typedef struct RenderCommand
{
  Mesh* mesh;
  Material* material;
  Mat4 transform;
} RenderCommand;

OTTERRENDER_API int render_command_compare(
    const RenderCommand* a, const RenderCommand* b);

