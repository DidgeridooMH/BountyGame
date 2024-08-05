#pragma once

typedef enum ComponentType
{
  CT_POSITION,
  CT_COUNT
} ComponentType;

#if COMPONENT_COUNT >= 64
#error "No more than 64 components are supported"
#endif
