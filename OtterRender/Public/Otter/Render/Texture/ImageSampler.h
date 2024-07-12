#pragma once

#include <vulkan/vulkan.h>

#include "Otter/Render/Texture/Image.h"
#include "Otter/Render/export.h"

typedef struct ImageSampler
{
  VkSampler sampler;
  VkImageView view;
} ImageSampler;

OTTERRENDER_API bool image_sampler_create(
    Image* image, VkDevice device, ImageSampler* sampler);

OTTERRENDER_API void image_sampler_destroy(
    ImageSampler* sampler, VkDevice device);
