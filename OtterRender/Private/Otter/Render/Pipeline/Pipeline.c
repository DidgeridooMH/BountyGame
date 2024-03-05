#include "Otter/Render/Pipeline/Pipeline.h"

#include "Otter/Util/File.h"

VkShaderModule pipeline_load_shader_module(
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
