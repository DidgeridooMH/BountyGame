#include "Otter/Render/Pipeline/PbrPipeline.h"

#include "Otter/Math/Vec.h"
#include "Otter/Render/Pipeline/Pipeline.h"
#include "Otter/Render/RenderStack.h"
#include "Otter/Util/Log.h"

bool pbr_pipeline_create(
    VkDevice logicalDevice, VkRenderPass renderPass, PbrPipeline* pipeline)
{
  VkShaderModule vertexShader =
      pipeline_load_shader_module("Shaders/pbr.vert.spv", logicalDevice);
  if (vertexShader == VK_NULL_HANDLE)
  {
    return false;
  }

  VkShaderModule fragShader =
      pipeline_load_shader_module("Shaders/pbr.frag.spv", logicalDevice);
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
      .stride    = sizeof(Vec3),
      .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
  };
  VkVertexInputAttributeDescription attributeDescription[]   = {{.binding = 0,
        .location                                                         = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0}};
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
      .lineWidth = 1.0};

  VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .minSampleShading     = 1.0f};

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {
      .blendEnable         = true,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp        = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp        = VK_BLEND_OP_ADD,
      .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                      | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT};

  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
      .sType         = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = false,
      .attachmentCount = 1,
      .pAttachments    = &colorBlendAttachment};

  VkDescriptorSetLayoutBinding layoutBindings[] = {
      {.binding            = 0,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 1,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 2,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 3,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT},
      {.binding            = 4,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT}};

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
      .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pBindings    = layoutBindings,
      .bindingCount = _countof(layoutBindings)};
  if (vkCreateDescriptorSetLayout(logicalDevice, &descriptorSetLayoutCreateInfo,
          NULL, &pipeline->descriptorSetLayouts))
  {
    return false;
  }

  VkPipelineLayoutCreateInfo layoutCreateInfo = {
      .sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pSetLayouts    = &pipeline->descriptorSetLayouts,
      .setLayoutCount = 1};
  if (vkCreatePipelineLayout(
          logicalDevice, &layoutCreateInfo, NULL, &pipeline->layout)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to create pipeline layout.");
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    vkDestroyShaderModule(logicalDevice, fragShader, NULL);
    return false;
  }

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
      .layout              = pipeline->layout,
      .renderPass          = renderPass,
      .subpass             = 1};

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

void pbr_pipeline_destroy(PbrPipeline* pipeline, VkDevice logicalDevice)
{
  vkDestroyDescriptorSetLayout(
      logicalDevice, pipeline->descriptorSetLayouts, NULL);
  vkDestroyPipelineLayout(logicalDevice, pipeline->layout, NULL);
  vkDestroyPipeline(logicalDevice, pipeline->pipeline, NULL);
}

void pbr_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    RenderStack* renderStack, PbrPipeline* pipeline)
{

  VkDescriptorSetAllocateInfo attachmentDescriptorSetAllocInfo = {
      .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool     = descriptorPool,
      .pSetLayouts        = &pipeline->descriptorSetLayouts,
      .descriptorSetCount = 1};
  VkDescriptorSet attachmentDescriptorSet;
  if (vkAllocateDescriptorSets(logicalDevice, &attachmentDescriptorSetAllocInfo,
          &attachmentDescriptorSet)
      != VK_SUCCESS)
  {
    LOG_ERROR("WARN: Unable to allocate descriptors");
  }

  VkDescriptorImageInfo attachmentDescriptors[] = {
      {.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .imageView = renderStack->bufferAttachments[RSL_POSITION],
          .sampler   = VK_NULL_HANDLE},
      {.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .imageView = renderStack->bufferAttachments[RSL_NORMAL],
          .sampler   = VK_NULL_HANDLE},
      {.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .imageView = renderStack->bufferAttachments[RSL_COLOR],
          .sampler   = VK_NULL_HANDLE},
      {.imageLayout  = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          .imageView = renderStack->bufferAttachments[RSL_MATERIAL],
          .sampler   = VK_NULL_HANDLE}};

  VkWriteDescriptorSet attachmentWrites[_countof(attachmentDescriptors)] = {0};
  for (uint32_t i = 0; i < _countof(attachmentDescriptors); i++)
  {
    attachmentWrites[i].sType          = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    attachmentWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    attachmentWrites[i].descriptorCount = 1;
    attachmentWrites[i].dstBinding      = i;
    attachmentWrites[i].dstSet          = attachmentDescriptorSet;
    attachmentWrites[i].pImageInfo      = attachmentDescriptors + i;
  }

  vkUpdateDescriptorSets(
      logicalDevice, _countof(attachmentWrites), attachmentWrites, 0, NULL);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
      pipeline->layout, 0, 1, &attachmentDescriptorSet, 0, NULL);
}
 
