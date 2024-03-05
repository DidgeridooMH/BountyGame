#include "Otter/Render/Memory/MemoryType.h"

bool memory_type_find(uint32_t typeFilter,
    VkPhysicalDevice physicalDevice, VkMemoryPropertyFlags properties,
    uint32_t* memoryIndex)
{
  VkPhysicalDeviceMemoryProperties physicalMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(
      physicalDevice, &physicalMemoryProperties);

  for (uint32_t i = 0; i < physicalMemoryProperties.memoryTypeCount; i++)
  {
    bool isCorrectType = typeFilter & (1 << i);
    bool hasCorrectProperties =
        (physicalMemoryProperties.memoryTypes[i].propertyFlags & properties)
        == properties;
    if (isCorrectType && hasCorrectProperties)
    {
      *memoryIndex = i;
      return true;
    }
  }

  return false;
}
