#pragma once

#include "Otter/Render/Pipeline.h"

typedef struct PbrMaterial
{
  Pipeline pipeline;
} PbrMaterial;

bool pbr_material_create(
    VkDevice logicalDevice, VkRenderPass renderPass, PbrMaterial* material);
void pbr_material_destroy(PbrMaterial* material, VkDevice logicalDevice);
