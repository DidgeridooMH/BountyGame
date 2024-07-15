#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Mesh.h"
#include "Otter/Render/Texture/ImageSampler.h"

typedef struct RenderCommand
{
  Mesh* mesh;
  ImageSampler* albedo;
  ImageSampler* normal;
  ImageSampler* metallicRoughness;
  ImageSampler* ao;
  Mat4 transform;
} RenderCommand;
