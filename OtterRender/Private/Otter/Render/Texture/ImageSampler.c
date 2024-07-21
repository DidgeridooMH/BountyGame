#include "Otter/Render/Texture/ImageSampler.h"

#include "Otter/Util/Log.h"

bool image_sampler_create(Image* image, VkDevice device, ImageSampler* sampler)
{
  VkImageViewCreateInfo viewCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = image->image,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .format           = image->format,
      .components       = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g            = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b            = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a            = VK_COMPONENT_SWIZZLE_IDENTITY},
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel                = 0,
          .levelCount                  = image->mipLevels,
          .baseArrayLayer              = 0,
          .layerCount                  = 1}};

  if (vkCreateImageView(device, &viewCreateInfo, NULL, &sampler->view)
      != VK_SUCCESS)
  {
    LOG_WARNING("Could not create image view for sampler.");
    return false;
  }

  VkSamplerCreateInfo samplerCreateInfo = {
      .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter               = VK_FILTER_LINEAR,
      .minFilter               = VK_FILTER_LINEAR,
      .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
      .anisotropyEnable        = VK_TRUE,
      .maxAnisotropy           = 16,
      .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
      .compareEnable           = VK_FALSE,
      .compareOp               = VK_COMPARE_OP_ALWAYS,
      .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .mipLodBias              = 0.0f,
      .minLod                  = 0.0f,
      .maxLod                  = image->mipLevels};

  if (vkCreateSampler(device, &samplerCreateInfo, NULL, &sampler->sampler)
      != VK_SUCCESS)
  {
    LOG_WARNING("Could not create sampler for image.");
    vkDestroyImageView(device, sampler->view, NULL);
    return false;
  }

  return true;
}

void image_sampler_destroy(ImageSampler* sampler, VkDevice device)
{
  vkDestroySampler(device, sampler->sampler, NULL);
  vkDestroyImageView(device, sampler->view, NULL);
}
