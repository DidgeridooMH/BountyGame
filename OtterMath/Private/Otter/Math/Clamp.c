#include "Otter/Math/Clamp.h"

static uint32_t max(uint32_t a, uint32_t b)
{
  return a > b ? a : b;
}

static uint32_t min(uint32_t a, uint32_t b)
{
  return a < b ? a : b;
}

uint32_t clamp(uint32_t value, uint32_t minimum, uint32_t maximum)
{
  return max(min(value, maximum), minimum);
}
