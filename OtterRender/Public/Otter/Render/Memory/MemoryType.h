#pragma once

#include <vulkan/vulkan.h>

bool memory_type_find(uint32_t typeFilter, VkPhysicalDevice physicalDevice,
    VkMemoryPropertyFlags properties, uint32_t* memoryIndex);
