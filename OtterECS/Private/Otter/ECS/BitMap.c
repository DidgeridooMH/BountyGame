#include "Otter/ECS/BitMap.h"

void bit_map_create(BitMap* map)
{
  auto_array_create(map, sizeof(BitMapSlot));
}

void bit_map_destroy(BitMap* map)
{
  auto_array_destroy(map);
}

void bit_map_set_bit(BitMap* map, uint64_t slot, uint64_t bit, bool value)
{
  BitMapSlot* mask = (BitMapSlot*) auto_array_get(map, slot);
  if (value)
  {
    *mask |= ((BitMapSlot) 1) << bit;
  }
  else
  {
    *mask &= ~(((BitMapSlot) 1) << bit);
  }
}

bool bit_map_get_bit(BitMap* map, uint64_t slot, uint64_t bit)
{
  BitMapSlot* mask = (BitMapSlot*) auto_array_get(map, slot);
  return (*mask & (1ULL << bit)) > 0;
}

void bit_map_set(BitMap* map, uint64_t index, bool value)
{
  uint64_t maskIndex = index / BIT_MAP_MASK_ENTRY_SIZE;
  uint64_t maskBit   = index % BIT_MAP_MASK_ENTRY_SIZE;
  bit_map_set_bit(map, maskIndex, maskBit, value);
}

bool bit_map_get(BitMap* map, uint64_t index)
{
  uint64_t maskIndex = index / BIT_MAP_MASK_ENTRY_SIZE;
  uint64_t maskBit   = index % BIT_MAP_MASK_ENTRY_SIZE;
  return bit_map_get_bit(map, maskIndex, maskBit);
}

bool bit_map_find_first_unset(BitMap* map, uint64_t* index)
{
  for (uint64_t i = 0; i < map->size; i++)
  {
    BitMapSlot mask = *(BitMapSlot*) auto_array_get(map, i);
    if (mask < ~((BitMapSlot) 0))
    {
      for (uint64_t j = 0; j < BIT_MAP_MASK_ENTRY_SIZE; j++)
      {
        if ((mask & (((BitMapSlot) 1) << j)) == 0)
        {
          *index = i * BIT_MAP_MASK_ENTRY_SIZE + j;
          return true;
        }
      }
    }
  }

  return false;
}

uint64_t bit_map_expand(BitMap* map)
{
  BitMapSlot* newMask = auto_array_allocate(map);
  *newMask            = ~((BitMapSlot) 0);
  return BIT_MAP_MASK_ENTRY_SIZE;
}

uint64_t bit_map_compact(BitMap* map)
{
  uint64_t compacted = 0;

  while (map->size > 0 && *(uint64_t*) auto_array_get(map, map->size - 1) == 0)
  {
    auto_array_pop(map);
    compacted += BIT_MAP_MASK_ENTRY_SIZE;
  }

  return compacted;
}
 
