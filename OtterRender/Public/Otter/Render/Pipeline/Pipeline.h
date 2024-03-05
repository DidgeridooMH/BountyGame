#pragma once

#include <vulkan/vulkan.h>

VkShaderModule pipeline_load_shader_module(
    const char* file, VkDevice logicalDevice);
