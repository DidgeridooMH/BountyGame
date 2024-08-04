#include "Otter/ECS/ComponentList.h"

void component_list_create(ComponentList* list, uint64_t componentSize)
{
  bit_map_create(&list->usedMask);
  auto_array_create(&list->components, componentSize);
}

void component_list_destroy(ComponentList* list)
{
  bit_map_destroy(&list->usedMask);
  auto_array_destroy(&list->components);
}

void* component_list_allocate(ComponentList* list)
{
  uint64_t index;
  if (!bit_map_find_first_unset(&list->usedMask, &index))
  {
    index               = list->components.size;
    uint64_t newEntries = bit_map_expand(&list->usedMask);
    auto_array_allocate_many(&list->components, newEntries);
  }

  bit_map_set(&list->usedMask, index, true);

  return auto_array_get(&list->components, index);
}

void component_list_deallocate(ComponentList* list, uint64_t index)
{
  bit_map_set(&list->usedMask, index, false);

  uint64_t compactedEntries = bit_map_compact(&list->usedMask);

  if (compactedEntries > 0)
  {
    auto_array_pop_many(&list->components, compactedEntries);
  }
}

void* component_list_get(ComponentList* list, uint64_t index)
{
  if (index >= list->components.size || !bit_map_get(&list->usedMask, index))
  {
    return NULL;
  }

  return auto_array_get(&list->components, index);
}
