#pragma once

inline uint32_t clamp(uint32_t value, uint32_t minimum, uint32_t maximum)
{
  return max(min(value, maximum), minimum);
}
