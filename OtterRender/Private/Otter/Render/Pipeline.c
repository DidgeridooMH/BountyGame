#include "Otter/Render/Pipeline.h"

#include "Otter/Render/Mesh.h"
#include "Otter/Util/File.h"
#include "Otter/Util/Math/Vec.h"

static VkShaderModule pipeline_load_shader_module(
    const char* file, VkDevice logicalDevice)
{
  uint64_t shaderLength = 0;
  char* shaderCode      = file_load(file, &shaderLength);
  if (shaderCode == NULL)
  {
    fprintf(stderr, "Unable to find vertex shader\n");
    return VK_NULL_HANDLE;
  }

  VkShaderModuleCreateInfo shaderCreateInfo = {
      .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pCode    = (uint32_t*) shaderCode,
      .codeSize = shaderLength};

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  if (vkCreateShaderModule(
          logicalDevice, &shaderCreateInfo, NULL, &shaderModule)
      != VK_SUCCESS)
  {
    free(shaderCode);
    fprintf(stderr, "Unable to create shader module.\n");
    return VK_NULL_HANDLE;
  }

  free(shaderCode);

  return shaderModule;
}

Pipeline* pipeline_create(
    const char* name, VkDevice logicalDevice, VkRenderPass renderPass)
{
  char vertexShaderName[256] = {0};
  snprintf(
      vertexShaderName, sizeof(vertexShaderName), "Shaders/%s.vert.spv", name);
  VkShaderModule vertexShader =
      pipeline_load_shader_module(vertexShaderName, logicalDevice);
  if (vertexShader == VK_NULL_HANDLE)
  {
    return NULL;
  }

  char fragShaderName[256] = {0};
  snprintf(
      fragShaderName, sizeof(vertexShaderName), "Shaders/%s.frag.spv", name);
  VkShaderModule fragShader =
      pipeline_load_shader_module(fragShaderName, logicalDevice);
  if (fragShader == VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    return NULL;
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
          .format   = VK_FORMAT_R32G32B32_SFLOAT,
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
      .logicOpEnable   = false,
      .attachmentCount = 1,
      .pAttachments    = &colorBlendAttachment};

  VkPipelineLayoutCreateInfo layoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  Pipeline* pipeline = malloc(sizeof(Pipeline));
  if (pipeline == NULL)
  {
    fprintf(stderr, "OOM\n");
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    vkDestroyShaderModule(logicalDevice, fragShader, NULL);
    return NULL;
  }

  if (vkCreatePipelineLayout(
          logicalDevice, &layoutCreateInfo, NULL, &pipeline->layout)
      != VK_SUCCESS)
  {
    fprintf(stderr, "Unable to create pipeline layout.\n");
    vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
    vkDestroyShaderModule(logicalDevice, fragShader, NULL);
    free(pipeline);
    return NULL;
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
    free(pipeline);
    return NULL;
  }

  vkDestroyShaderModule(logicalDevice, vertexShader, NULL);
  vkDestroyShaderModule(logicalDevice, fragShader, NULL);

  return pipeline;
}

void pipeline_destroy(Pipeline* pipeline, VkDevice logicalDevice)
{
  vkDestroyPipelineLayout(logicalDevice, pipeline->layout, NULL);
  vkDestroyPipeline(logicalDevice, pipeline->pipeline, NULL);
  free(pipeline);
}
