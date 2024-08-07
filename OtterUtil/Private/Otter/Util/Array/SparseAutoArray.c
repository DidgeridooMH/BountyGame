#include "Otter/Util/Array/SparseAutoArray.h"

void sparse_auto_array_create(SparseAutoArray* list, uint64_t componentSize)
{
  bit_map_create(&list->usedMask);
  auto_array_create(&list->components, componentSize);
}

void sparse_auto_array_destroy(SparseAutoArray* list)
{
  bit_map_destroy(&list->usedMask);
  auto_array_destroy(&list->components);
}

uint64_t sparse_auto_array_allocate(SparseAutoArray* list)
{
  uint64_t index;
  if (!bit_map_find_first_unset(&list->usedMask, &index))
  {
    index               = list->components.size;
    uint64_t newEntries = bit_map_expand(&list->usedMask);
    auto_array_allocate_many(&list->components, newEntries);
  }

  bit_map_set(&list->usedMask, index, true);

  return index;
}

uint64_t sparse_auto_array_deallocate(SparseAutoArray* list, uint64_t index)
{
  bit_map_set(&list->usedMask, index, false);

  uint64_t compactedEntries = bit_map_compact(&list->usedMask);

  if (compactedEntries > 0)
  {
    auto_array_pop_many(&list->components, compactedEntries);
  }

  return compactedEntries;
}

void* sparse_auto_array_get(SparseAutoArray* list, uint64_t index)
{
  if (index >= list->components.size || !bit_map_get(&list->usedMask, index))
  {
    return NULL;
  }

  return auto_array_get(&list->components, index);
}
