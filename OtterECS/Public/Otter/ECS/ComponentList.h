#pragma once

#include <stdint.h>

#include "Otter/ECS/BitMap.h"
#include "Otter/Util/AutoArray.h"

typedef struct ComponentList
{
  BitMap usedMask;
  AutoArray components;
} ComponentList;

void component_list_create(ComponentList* list, uint64_t componentSize);

void component_list_destroy(ComponentList* list);

void* component_list_allocate(ComponentList* list);

void component_list_deallocate(ComponentList* list, uint64_t index);

void* component_list_get(ComponentList* list, uint64_t index);

