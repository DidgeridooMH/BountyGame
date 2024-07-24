#include "Otter/Render/RenderPass/LightingPass.h"

#include "Otter/Util/Log.h"

bool lighting_pass_create_render_pass(
    VkRenderPass* renderPass, VkFormat format, VkDevice logicalDevice)
{
  VkAttachmentDescription attachments[] = {
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = VK_FORMAT_R16G16B16A16_SFLOAT,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_LOAD,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .finalLayout   = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      },
      {
          .format        = format,
          .samples       = VK_SAMPLE_COUNT_1_BIT,
          .loadOp        = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp       = VK_ATTACHMENT_STORE_OP_STORE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      },
  };

  VkAttachmentReference colorAttachmentRef[] = {{
      .attachment = 4,
      .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  }};

  VkAttachmentReference lightingInputAttachmentRef[] = {
      {.attachment = GBL_POSITION,
          .layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {.attachment = GBL_NORMAL,
          .layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {.attachment = GBL_COLOR,
          .layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
      {.attachment = GBL_MATERIAL,
          .layout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}};

  VkSubpassDescription subpasses[] = {{
      .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = _countof(colorAttachmentRef),
      .pColorAttachments    = colorAttachmentRef,
      .inputAttachmentCount = _countof(lightingInputAttachmentRef),
      .pInputAttachments    = lightingInputAttachmentRef,
  }};

  VkSubpassDependency subpassDependencies[] = {{
      .srcSubpass      = VK_SUBPASS_EXTERNAL,
      .dstSubpass      = 0,
      .srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask    = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      .srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask   = VK_ACCESS_SHADER_READ_BIT,
      .dependencyFlags = 0,
  }};

  VkRenderPassCreateInfo renderPassInfo = {
      .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = _countof(attachments),
      .pAttachments    = attachments,
      .subpassCount    = _countof(subpasses),
      .pSubpasses      = subpasses,
      .pDependencies   = subpassDependencies,
      .dependencyCount = _countof(subpassDependencies),
  };

  if (vkCreateRenderPass(logicalDevice, &renderPassInfo, NULL, renderPass)
      != VK_SUCCESS)
  {
    LOG_ERROR("Failed to create render pass");
    return false;
  }

  return true;
}

bool lighting_pass_create(LightingPass* lightingPass, GBufferPass* gbufferPass,
    VkRenderPass renderPass, VkImage renderImage, VkExtent2D extents,
    VkFormat renderFormat, VkPhysicalDevice physicalDevice,
    VkDevice logicalDevice)
{
  VkImageViewCreateInfo finalImageViewCreateInfo = {
      .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image            = renderImage,
      .format           = renderFormat,
      .viewType         = VK_IMAGE_VIEW_TYPE_2D,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseArrayLayer              = 0,
          .layerCount                  = 1,
          .baseMipLevel                = 0,
          .levelCount                  = 1}};

  if (vkCreateImageView(logicalDevice, &finalImageViewCreateInfo, NULL,
          &lightingPass->finalImage)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to get swapchain images.");
    return false;
  }

  lightingPass->imageSize = extents;

  VkImageView attachments[] = {gbufferPass->bufferAttachments[GBL_POSITION],
      gbufferPass->bufferAttachments[GBL_NORMAL],
      gbufferPass->bufferAttachments[GBL_COLOR],
      gbufferPass->bufferAttachments[GBL_MATERIAL], lightingPass->finalImage};

  LOG_DEBUG("GBUFFER: %p", gbufferPass->gBufferImage.image);
  LOG_DEBUG("FINAL: %p", renderImage);

  VkFramebufferCreateInfo framebufferCreateInfo = {
      .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass      = renderPass,
      .attachmentCount = _countof(attachments),
      .pAttachments    = attachments,
      .width           = extents.width,
      .height          = extents.height,
      .layers          = 1};

  if (vkCreateFramebuffer(logicalDevice, &framebufferCreateInfo, NULL,
          &lightingPass->framebuffer)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to create framebuffer");
    return false;
  }

  return true;
}

void lighting_pass_destroy(LightingPass* lightingPass, VkDevice logicalDevice)
{
  vkDestroyImageView(logicalDevice, lightingPass->finalImage, NULL);
  vkDestroyFramebuffer(logicalDevice, lightingPass->framebuffer, NULL);
}

