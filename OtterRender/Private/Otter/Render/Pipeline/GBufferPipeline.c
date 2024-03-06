#include "Otter/Render/Pipeline/GBufferPipeline.h"

#include "Otter/Render/Mesh.h"
#include "Otter/Render/Pipeline/Pipeline.h"
#include "Otter/Render/RenderStack.h"

bool g_buffer_pipeline_create(
    VkDevice logicalDevice, VkRenderPass renderPass, GBufferPipeline* pipeline)
{
  VkShaderModule vertexShader =
      pipeline_load_shader_module("Shaders/gbuffer.vert.spv", logicalDevice);
  if (vertexShader == VK_NULL_HANDLE)
  {
    return false;
  }

  VkShaderModule fragShader =
      pipeline_load_shader_module("Shaders/gbuffer.frag.spv", logicalDevice);
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
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(MeshVertex, tangent)},
      {.binding     = 0,
          .location = 3,
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
          .offset   = offsetof(MeshVertex, bitangent)},
      {.binding     = 0,
          .location = 4,
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
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

  VkPipelineMultisampleStateCreateInfo multisamplingStateCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .minSampleShading     = 1.0f};

  VkPipelineColorBlendAttachmentState
      colorBlendAttachment[NUM_OF_GBUFFER_LAYERS] = {0};
  for (int i = 0; i < _countof(colorBlendAttachment); i++)
  {
    colorBlendAttachment[i].blendEnable         = true;
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
      .logicOpEnable   = false,
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
    fprintf(stderr, "Unable to create pipeline layout.\n");
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
  vkDestroyPipelineLayout(logicalDevice, pipeline->layout, NULL);
  vkDestroyPipeline(logicalDevice, pipeline->pipeline, NULL);
}
