#pragma once

#include "Otter/Math/Vec.h"
#include "Otter/Render/Texture/ImageSampler.h"

typedef struct MaterialConstant
{
  Vec4 baseColorFactor;
  float metallicFactor;
  float roughnessFactor;
  float occlusionStrength;
  uint32_t useBaseColorTexture;
  uint32_t useNormalTexture;
  uint32_t useMetallicRoughnessTexture;
  uint32_t useOcclusionTexture;
} MaterialConstant;

typedef struct Material
{
  MaterialConstant constant;
  ImageSampler* baseColorTexture;
  ImageSampler* metallicRoughnessTexture;
  ImageSampler* occlusionTexture;
  ImageSampler* normalTexture;
} Material;
