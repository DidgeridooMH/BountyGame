#include "Otter/Render/Pipeline/RayTracingPipeline.h"

#include <vulkan/vulkan_core.h>

#include "Otter/Render/Pipeline/Pipeline.h"
#include "Otter/Render/RayTracing/AccelerationStructure.h"
#include "Otter/Render/RayTracing/RayTracingFunctions.h"
#include "Otter/Util/Log.h"

bool ray_tracing_pipeline_create(const char* shaderDirectory,
    VkDevice logicalDevice, RayTracingPipeline* pipeline)
{
  char shaderPath[MAX_PATH] = {0};
  snprintf(shaderPath, MAX_PATH, "%s/rt.rgen.spv", shaderDirectory);
  VkShaderModule raygenShader =
      pipeline_load_shader_module(shaderPath, logicalDevice);
  if (raygenShader == VK_NULL_HANDLE)
  {
    return false;
  }

  snprintf(shaderPath, MAX_PATH, "%s/rt.rmiss.spv", shaderDirectory);
  VkShaderModule rmissShader =
      pipeline_load_shader_module(shaderPath, logicalDevice);
  if (rmissShader == VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(logicalDevice, raygenShader, NULL);
    return false;
  }

  snprintf(shaderPath, MAX_PATH, "%s/rt.rchit.spv", shaderDirectory);
  VkShaderModule rchitShader =
      pipeline_load_shader_module(shaderPath, logicalDevice);
  if (rchitShader == VK_NULL_HANDLE)
  {
    vkDestroyShaderModule(logicalDevice, raygenShader, NULL);
    vkDestroyShaderModule(logicalDevice, rmissShader, NULL);
    return false;
  }

  VkPipelineShaderStageCreateInfo shaderStagesCreateInfo[] = {
      {.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .module = raygenShader,
          .pName  = "main",
          .stage  = VK_SHADER_STAGE_RAYGEN_BIT_KHR},
      {.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .module = rmissShader,
          .pName  = "main",
          .stage  = VK_SHADER_STAGE_MISS_BIT_KHR},
      {.sType     = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .module = rchitShader,
          .pName  = "main",
          .stage  = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
  };

  VkRayTracingShaderGroupCreateInfoKHR rayTracingShaderGroupCreateInfo[3];
  for (int i = 0; i < 3; i++)
  {
    rayTracingShaderGroupCreateInfo[i] = (VkRayTracingShaderGroupCreateInfoKHR){
        .sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        .type  = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        .generalShader      = VK_SHADER_UNUSED_KHR,
        .closestHitShader   = VK_SHADER_UNUSED_KHR,
        .anyHitShader       = VK_SHADER_UNUSED_KHR,
        .intersectionShader = VK_SHADER_UNUSED_KHR,
    };
  }

  rayTracingShaderGroupCreateInfo[0].generalShader = 0;
  rayTracingShaderGroupCreateInfo[1].generalShader = 1;
  rayTracingShaderGroupCreateInfo[2].type =
      VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  rayTracingShaderGroupCreateInfo[2].generalShader    = VK_SHADER_UNUSED_KHR;
  rayTracingShaderGroupCreateInfo[2].closestHitShader = 2;

  VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

  VkDescriptorSetLayoutBinding layoutBindings[] = {
      {.binding            = 0,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
          .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR},
      {.binding            = 1,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR},
      {.binding            = 2,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR},
      {.binding            = 3,
          .descriptorCount = 1,
          .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .stageFlags      = VK_SHADER_STAGE_RAYGEN_BIT_KHR},
  };

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
    vkDestroyShaderModule(logicalDevice, raygenShader, NULL);
    vkDestroyShaderModule(logicalDevice, rmissShader, NULL);
    vkDestroyShaderModule(logicalDevice, rchitShader, NULL);
    return false;
  }

  VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
      .sType      = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
      .stageCount = _countof(shaderStagesCreateInfo),
      .pStages    = shaderStagesCreateInfo,
      .groupCount = _countof(rayTracingShaderGroupCreateInfo),
      .pGroups    = rayTracingShaderGroupCreateInfo,
      .maxPipelineRayRecursionDepth = 1,
      .layout                       = pipeline->layout,
  };

  if (_vkCreateRayTracingPipelinesKHR(logicalDevice, VK_NULL_HANDLE,
          VK_NULL_HANDLE, 1, &rayTracingPipelineCreateInfo, NULL,
          &pipeline->pipeline)
      != VK_SUCCESS)
  {
    LOG_ERROR("Unable to create ray tracing pipeline.");
    vkDestroyShaderModule(logicalDevice, raygenShader, NULL);
    vkDestroyShaderModule(logicalDevice, rmissShader, NULL);
    vkDestroyShaderModule(logicalDevice, rchitShader, NULL);
    return false;
  }

  vkDestroyShaderModule(logicalDevice, raygenShader, NULL);
  vkDestroyShaderModule(logicalDevice, rmissShader, NULL);
  vkDestroyShaderModule(logicalDevice, rchitShader, NULL);

  return true;
}

void ray_tracing_pipeline_destroy(
    RayTracingPipeline* pipeline, VkDevice logicalDevice)
{
  vkDestroyPipeline(logicalDevice, pipeline->pipeline, NULL);
  vkDestroyPipelineLayout(logicalDevice, pipeline->layout, NULL);
  vkDestroyDescriptorSetLayout(
      logicalDevice, pipeline->descriptorSetLayouts, NULL);
}

void ray_tracing_pipeline_write_descriptor_set(VkCommandBuffer commandBuffer,
    VkDescriptorPool descriptorPool, VkDevice logicalDevice,
    RenderStack* renderStack, AccelerationStructure* accelerationStructure,
    RayTracingPipeline* pipeline)
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
      {.imageLayout  = VK_IMAGE_LAYOUT_GENERAL,
          .imageView = renderStack->lightingPass.shadowMapView},
      {.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
          .imageView =
              renderStack->gbufferPass.bufferAttachments[GBL_POSITION]},
      {.imageLayout  = VK_IMAGE_LAYOUT_GENERAL,
          .imageView = renderStack->gbufferPass.bufferAttachments[GBL_NORMAL]},
  };

  const VkAccelerationStructureKHR as =
      *(VkAccelerationStructureKHR*) auto_array_get(
          &accelerationStructure->topLevel.accelerationStructures, 0);
  VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureInfo = {
      .sType =
          VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
      .pAccelerationStructures    = &as,
      .accelerationStructureCount = 1,
  };

  VkWriteDescriptorSet attachmentWrites[_countof(attachmentDescriptors) + 1] = {
      {
          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorType  = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
          .dstBinding      = 0,
          .dstSet          = attachmentDescriptorSet,
          .descriptorCount = 1,
          .pNext           = &accelerationStructureInfo,
      },
      {
          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .dstBinding      = 1,
          .dstSet          = attachmentDescriptorSet,
          .descriptorCount = 1,
          .pImageInfo      = &attachmentDescriptors[0],
      },
      {
          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .dstBinding      = 2,
          .dstSet          = attachmentDescriptorSet,
          .descriptorCount = 1,
          .pImageInfo      = &attachmentDescriptors[1],
      },
      {
          .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .dstBinding      = 3,
          .dstSet          = attachmentDescriptorSet,
          .descriptorCount = 1,
          .pImageInfo      = &attachmentDescriptors[2],
      }};

  vkUpdateDescriptorSets(
      logicalDevice, _countof(attachmentWrites), attachmentWrites, 0, NULL);

  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
      pipeline->layout, 0, 1, &attachmentDescriptorSet, 0, NULL);
}

