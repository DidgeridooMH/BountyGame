#include "Otter/Render/Pipeline/Pipeline.h"

#include "Otter/Util/File.h"
#include "Otter/Util/Log.h"

VkShaderModule pipeline_load_shader_module(
    const char* file, VkDevice logicalDevice)
{
  uint64_t shaderLength = 0;
  char* shaderCode      = file_load(file, &shaderLength);
  if (shaderCode == NULL)
  {
    LOG_ERROR("Unable to find vertex shader");
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
    LOG_ERROR("Unable to create shader module.");
    return VK_NULL_HANDLE;
  }

  free(shaderCode);

  return shaderModule;
}
