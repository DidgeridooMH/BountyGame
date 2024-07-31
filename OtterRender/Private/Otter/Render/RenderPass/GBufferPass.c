#include "Otter/Render/RenderPass/GBufferPass.h"

#include <vulkan/vulkan_core.h>

#include "Otter/Util/Log.h"

bool gbuffer_pass_create_render_pass(
    VkRenderPass* renderPass, VkDevice logicalDevice)
{
  VkAttachmentDescription attachments[] = {
      {
          .format        = VK_FORMAT_R32G32B32A32_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R8G8B8A8_UNORM,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
          .format         = VK_FORMAT_D32_SFLOAT,
          .samples        = VK_SAMPLE_COUNT_1_BIT,
          .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      }};

  VkAttachmentReference colorAttachmentRefs[] = {
      {
          .attachment = GBL_POSITION,
          .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .attachment = GBL_NORMAL,
          .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .attachment = GBL_COLOR,
          .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .attachment = GBL_MATERIAL,
          .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      }};

  VkAttachmentReference depthAttachmentRef = {
      .attachment = 4,
      .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
      .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount    = _countof(colorAttachmentRefs),
      .pColorAttachments       = colorAttachmentRefs,
      .pDepthStencilAttachment = &depthAttachmentRef,
  };

  VkSubpassDependency subpassDependencies[] = {{
      .srcSubpass   = VK_SUBPASS_EXTERNAL,
      .dstSubpass   = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                    | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                     | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
  }};

  VkRenderPassCreateInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = _countof(attachments),
      .pAttachments    = attachments,
      .subpassCount    = 1,
      .pSubpasses      = &subpass,
      .dependencyCount = _countof(subpassDependencies),
      .pDependencies   = subpassDependencies,
  };

  if (vkCreateRenderPass(logicalDevice, &renderPassInfo, NULL, renderPass)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to create color pass.");
    return false;
  }

  return true;
}

static bool gbuffer_pass_create_image(VkExtent2D extents, VkFormat format,
    Image* image, VkImageView* imageView, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  if (!image_create(extents, 1, format,
          VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
              | VK_IMAGE_USAGE_STORAGE_BIT,
          false, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice,
          logicalDevice, image))
  {
    return false;
  }

  VkImageViewCreateInfo gBufferImageViewCreateInfo = {
      .sType                         = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image                         = image->image,
      .viewType                      = VK_IMAGE_VIEW_TYPE_2D,
      .format                        = format,
      .subresourceRange.aspectMask   = VK_IMAGE_ASPECT_COLOR_BIT,
      .subresourceRange.baseMipLevel = 0,
      .subresourceRange.levelCount   = 1,
      .subresourceRange.baseArrayLayer = 0,
      .subresourceRange.layerCount     = 1,
  };
  return vkCreateImageView(
             logicalDevice, &gBufferImageViewCreateInfo, NULL, imageView)
      == VK_SUCCESS;
}

bool gbuffer_pass_create(GBufferPass* gbufferPass, VkRenderPass renderPass,
    VkExtent2D extents, VkPhysicalDevice physicalDevice, VkDevice logicalDevice)
{
  VkFormat gbufferFormats[] = {VK_FORMAT_R32G32B32A32_SFLOAT,
      VK_FORMAT_R16G16B16A16_SFLOAT, VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_R16G16B16A16_SFLOAT};
  for (int i = 0; i < NUM_OF_GBUFFER_PASS_LAYERS; i++)
  {
    if (!gbuffer_pass_create_image(extents, gbufferFormats[i],
            &gbufferPass->bufferImages[i], &gbufferPass->bufferAttachments[i],
            physicalDevice, logicalDevice))
    {
      return false;
    }
  }

  if (!image_create(extents, 1, VK_FORMAT_D32_SFLOAT,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, false,
          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, physicalDevice, logicalDevice,
          &gbufferPass->depthBuffer))
  {
    return false;
  }

  VkImageViewCreateInfo depthImageViewCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = gbufferPass->depthBuffer.image,
      .format           = VK_FORMAT_D32_SFLOAT,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseArrayLayer              = 0,
          .layerCount                  = 1,
          .baseMipLevel                = 0,
          .levelCount                  = 1}};
  if (vkCreateImageView(logicalDevice, &depthImageViewCreateInfo, NULL,
          &gbufferPass->bufferAttachments[4])
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to create depth attachment.");
    return false;
  }

  VkFramebufferCreateInfo framebufferCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = renderPass,
      .attachmentCount = _countof(gbufferPass->bufferAttachments),
      .pAttachments    = gbufferPass->bufferAttachments,
      .width           = extents.width,
      .height          = extents.height,
      .layers          = 1,
  };
  if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, NULL,
          &gbufferPass->framebuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Error: Unable to create framebuffer from image view.");
    return false;
  }

  return true;
}

void gbuffer_pass_destroy(GBufferPass* gbufferPass, VkDevice logicalDevice)
{
  vkDestroyFramebuffer(logicalDevice, gbufferPass->framebuffer, NULL);
  vkDestroyImageView(logicalDevice, gbufferPass->bufferAttachments[4], NULL);
  image_destroy(&gbufferPass->depthBuffer, logicalDevice);
  for (int layer = 0; layer < NUM_OF_GBUFFER_PASS_LAYERS; layer++)
  {
    vkDestroyImageView(
        logicalDevice, gbufferPass->bufferAttachments[layer], NULL);
    image_destroy(&gbufferPass->bufferImages[layer], logicalDevice);
  }
}

