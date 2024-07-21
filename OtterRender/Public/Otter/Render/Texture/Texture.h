#pragma once

#include "Otter/Render/Texture/Image.h"
#include "Otter/Render/Texture/ImageSampler.h"
#include "Otter/Render/export.h"

typedef struct Texture
{
  Image image;
  ImageSampler sampler;
} Texture;

typedef enum TexureType
{
  TT_COLOR,
  TT_NONCOLOR
} TextureType;

OTTERRENDER_API bool texture_create(Texture* texture, const uint8_t* data,
    uint32_t width, uint32_t height, uint32_t channels, TextureType textureType,
    bool useMipMap, VkPhysicalDevice physicalDevice, VkDevice logicalDevice,
    VkCommandPool commandPool, VkQueue commandQueue);

OTTERRENDER_API void texture_destroy(Texture* texture, VkDevice logicalDevice);
