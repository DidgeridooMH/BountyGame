#include "Otter/Render/Material/PbrMaterial.h"

bool pbr_material_create(
    VkDevice logicalDevice, VkRenderPass renderPass, PbrMaterial* material)
{
  /*VkDescriptorSetLayoutBinding bindingLayouts[] = {{.binding = 0,
      .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      .descriptorCount = 1,
      .stageFlags      = VK_SHADER_STAGE_VERTEX_BIT}};

  if (!pipeline_create("pbr", bindingLayouts, _countof(bindingLayouts),
          logicalDevice, renderPass, &material->pipeline))
  {
    return false;
  }*/
  return false;
}

void pbr_material_destroy(PbrMaterial* material, VkDevice logicalDevice)
{
  g_buffer_pipeline_destroy(&material->pipeline, logicalDevice);
}
