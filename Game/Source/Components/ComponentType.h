#pragma once

typedef enum ComponentType
{
  CT_MESH,
  CT_MATERIAL,
  CT_VELOCITY,
  CT_COUNT
} ComponentType;

#if COMPONENT_COUNT >= 64
#error "No more than 64 components are supported"
#endif
