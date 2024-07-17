#include "Otter/Render/Pipeline/GBufferPipeline.h"

#include <vulkan/vulkan_core.h>

#include "Otter/Math/MatDef.h"
#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/Pipeline.h"
#include "Otter/Render/RenderStack.h"
#include "Otter/Render/Texture/ImageSampler.h"
#include "Otter/Render/Uniform/Material.h"
#include "Otter/Util/Log.h"

bool g_buffer_pipeline_create(const char* shaderDirectory,
    VkDevice logicalDevice, VkRenderPass renderPass, GBufferPipeline* pipeline)
{
  char shaderPath[MAX_PATH] = {0};
  snprintf(
      shaderPath, _countof(shaderPath), "%s/gbuffer.vert.spv", shaderDirectory);
  VkShaderModule vertexShader =
      pipeline_load_shader_module(shaderPath, logicalDevice);
  if (vertexShader == VK_NULL_HANDLE)
  {
    return false;
  }

  snprintf(
      shaderPath, _countof(shaderPath), "%s/gbuffer.frag.spv", shaderDirectory);
  VkShaderModule fragShader =
      pipeline_load_shader_module(shaderPath, logicalDevice);
  if (fragShader == VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    return false;
  }

  VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = {
      {.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .module = vertexShader,
          .pName  = "main",
          .stage  = VK_SHADER_STAGE_VERTEX_BIT},
      {.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .module = fragShader,
          .pName  = "main",
          .stage  = VK_SHADER_STAGE_FRAGMENT_BIT}};

  VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
      .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pDynamicStates    = dynamicStates,
      .dynamicStateCount = _countof(dynamicStates)};

  VkVertexInputBindingDescription vertexBindingDescription = {
      .binding   = 0,
      .stride    = sizeof(MeshVertex),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  VkVertexInputAttributeDescription attributeDescription[] = {
      {.binding     = 0,
          .location = 0,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(MeshVertex, position)},
      {.binding     = 0,
          .location = 1,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(MeshVertex, normal)},
      {.binding     = 0,
          .location = 2,
          .format   = VK_FORMAT_R32G32B32A32_SFLOAT,
          .offset   = offsetof(MeshVertex, tangent)},
      {.binding     = 0,
          .location = 3,
          .format   = VK_FORMAT_R32G32_SFLOAT,
          .offset   = offsetof(MeshVertex, uv)}};
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pVertexBindingDescriptions      = &vertexBindingDescription,
      .vertexBindingDescriptionCount   = 1,
      .pVertexAttributeDescriptions    = attributeDescription,
      .vertexAttributeDescriptionCount = _countof(attributeDescription)};

  VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
      .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST};

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount  = 1};

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
      .sType     = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .cullMode  = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f};

  VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .minSampleShading     = 1.0f};

  VkPipelineColorBlendAttachmentState colorBlendAttachment[G_BUFFER_LAYERS] = {
      0};
  // TODO: Fix transparency pipeline. This seems super complex.
  for (int i = 0; i < _countof(colorBlendAttachment); i++)
  {
    colorBlendAttachment[i].blendEnable = i == RSL_COLOR ? VK_TRUE : VK_FALSE;
    colorBlendAttachment[i].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment[i].dstColorBlendFactor =
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment[i].colorBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment[i].alphaBlendOp        = VK_BLEND_OP_ADD;
    colorBlendAttachment[i].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  }
  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = false,
      .attachmentCount = _countof(colorBlendAttachment),
      .pAttachments    = colorBlendAttachment};

  VkDescriptorSetLayoutBinding layoutBindings[] = {{.binding = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT}};
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pBindings    = layoutBindings,
      .bindingCount = _countof(layoutBindings)};
  if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo,
          NULL, &pipeline->descriptorSetLayouts[DSI_VP]))
  {
    return false;
  }

  VkDescriptorSetLayoutBinding materialLayoutBindings[] = {
      {.binding            = 0,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 2,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 3,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT}};
  VkDescriptorSetLayoutCreateInfo materialDescriptorSetLayoutCreateInfo = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pBindings    = materialLayoutBindings,
      .bindingCount = _countof(materialLayoutBindings)};
  if (vkCreateDescriptorSetLayout(logicalDevice,
          &materialDescriptorSetLayoutCreateInfo, NULL,
          &pipeline->descriptorSetLayouts[DSI_MATERIAL])
      != VK_SUCCESS)
  {
    vkDestroyDescriptorSetLayout(
        logicalDevice, pipeline->descriptorSetLayouts[DSI_VP], NULL);
    return false;
  }

  VkPushConstantRange pushConstantRange[] = {
      {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
          .offset  = 0,
          .size    = sizeof(Mat4)},
      {.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
          .offset  = sizeof(Mat4),
          .size    = sizeof(MaterialConstant)}};

  VkPipelineLayoutCreateInfo layoutCreateInfo = {
      .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pSetLayouts            = pipeline->descriptorSetLayouts,
      .setLayoutCount         = _countof(pipeline->descriptorSetLayouts),
      .pushConstantRangeCount = _countof(pushConstantRange),
      .pPushConstantRanges    = pushConstantRange};
  if (vkCreatePipelineLayout(
          logicalDevice, &layoutCreateInfo, NULL, &pipeline->layout)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to create pipeline layout.");
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    vkDestroyShaderModule(logicalDevice, fragShader, NULL);
    return false;
  }

  VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable       = VK_TRUE,
      .depthWriteEnable      = VK_TRUE,
      .depthCompareOp        = VK_COMPARE_OP_LESS,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable     = VK_FALSE};

  VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
      .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount          = _countof(shaderStagesCreateInfo),
      .pStages             = shaderStagesCreateInfo,
      .pVertexInputState   = &vertexInputCreateInfo,
      .pInputAssemblyState = &inputAssemblyCreateInfo,
      .pViewportState      = &viewportStateCreateInfo,
      .pRasterizationState = &rasterizationStateCreateInfo,
      .pMultisampleState   = &multisamplingStateCreateInfo,
      .pColorBlendState    = &colorBlendStateCreateInfo,
      .pDynamicState       = &dynamicStateCreateInfo,
      .pDepthStencilState  = &depthStencilCreateInfo,
      .layout              = pipeline->layout,
      .renderPass          = renderPass};

  if (vkCreateGraphicsPipelines(logicalDevice, NULL, 1, &pipelineCreateInfo,
          NULL, &pipeline->pipeline)
      != VK_SUCCESS)
  {
    vkDestroyPipelineLayout(logicalDevice, pipeline->layout, NULL);
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    vkDestroyShaderModule(logicalDevice, fragShader, NULL);
    return false;
  }

  vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
  vkDestroyShaderModule(logicalDevice, fragShader, NULL);

  return true;
}

void g_buffer_pipeline_destroy(
    GBufferPipeline* pipeline, VkDevice logicalDevice)
{
  for (int i = 0; i < _countof(pipeline->descriptorSetLayouts); i++)
  {
    vkDestroyDescriptorSetLayout(
        logicalDevice, pipeline->descriptorSetLayouts[i], NULL);
  }
  vkDestroyPipelineLayout(logicalDevice, pipeline->layout, NULL);
  vkDestroyPipeline(logicalDevice, pipeline->pipeline, NULL);
}

void g_buffer_pipeline_write_vp(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    GpuBuffer* vpBuffer, GBufferPipeline* pipeline)
{
  VkDescriptorSetAllocateInfo gBufferDescriptorSetAllocInfo = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = descriptorPool,
      .pSetLayouts        = &pipeline->descriptorSetLayouts[DSI_VP],
      .descriptorSetCount = 1};
  VkDescriptorSet gBufferDescriptorSet;
  if (vkAllocateDescriptorSets(
          logicalDevice, &gBufferDescriptorSetAllocInfo, &gBufferDescriptorSet)
      != VK_SUCCESS)
  {
    LOG_ERROR("WARN: Unable to allocate descriptors");
  }

  VkDescriptorBufferInfo mvpBufferInfo = {
      .buffer = vpBuffer->buffer, .offset = 0, .range = vpBuffer->size};
  VkWriteDescriptorSet descriptorWrites[] = {
      {.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .dstSet          = gBufferDescriptorSet,
          .dstBinding      = 0,
          .dstArrayElement = 0,
          .pBufferInfo     = &mvpBufferInfo}};
  vkUpdateDescriptorSets(
      logicalDevice, _countof(descriptorWrites), descriptorWrites, 0, NULL);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout, 0, 1, &gBufferDescriptorSet, 0, NULL);
}

// TODO: Rework this so that we aren't writing a descriptor set for each object.
void g_buffer_pipeline_write_material(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice, Material* material,
    GBufferPipeline* pipeline)
{
  VkDescriptorSetAllocateInfo gBufferDescriptorSetAllocInfo = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = descriptorPool,
      .pSetLayouts        = &pipeline->descriptorSetLayouts[DSI_MATERIAL],
      .descriptorSetCount = 1};
  VkDescriptorSet gBufferDescriptorSet;
  if (vkAllocateDescriptorSets(
          logicalDevice, &gBufferDescriptorSetAllocInfo, &gBufferDescriptorSet)
      != VK_SUCCESS)
  {
    LOG_ERROR("WARN: Unable to allocate descriptors");
  }

  VkDescriptorImageInfo albedoImageInfo = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView   = material->baseColorTexture->view,
      .sampler     = material->baseColorTexture->sampler};
  VkDescriptorImageInfo normalImageInfo = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView   = material->normalTexture->view,
      .sampler     = material->normalTexture->sampler};
  VkDescriptorImageInfo metallicRoughnessImageInfo = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView   = material->metallicRoughnessTexture->view,
      .sampler     = material->metallicRoughnessTexture->sampler};
  VkDescriptorImageInfo aoImageInfo = {
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .imageView   = material->occlusionTexture->view,
      .sampler     = material->occlusionTexture->sampler};
  VkWriteDescriptorSet descriptorWrites[] = {
      {.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .dstSet          = gBufferDescriptorSet,
          .dstBinding      = 0,
          .dstArrayElement = 0,
          .pImageInfo      = &albedoImageInfo},
      {.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .dstSet          = gBufferDescriptorSet,
          .dstBinding      = 1,
          .dstArrayElement = 0,
          .pImageInfo      = &normalImageInfo},
      {.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .dstSet          = gBufferDescriptorSet,
          .dstBinding      = 2,
          .dstArrayElement = 0,
          .pImageInfo      = &metallicRoughnessImageInfo},
      {.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .dstSet          = gBufferDescriptorSet,
          .dstBinding      = 3,
          .dstArrayElement = 0,
          .pImageInfo      = &aoImageInfo}};
  vkUpdateDescriptorSets(
      logicalDevice, _countof(descriptorWrites), descriptorWrites, 0, NULL);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout, 1, 1, &gBufferDescriptorSet, 0, NULL);

  vkCmdPushConstants(commandBuffer, pipeline->layout,
      VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Mat4), sizeof(MaterialConstant),
      &material->constant);
}
