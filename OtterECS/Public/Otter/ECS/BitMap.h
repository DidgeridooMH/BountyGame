#pragma once

#include "Otter/Util/AutoArray.h"

typedef AutoArray BitMap;
typedef uint64_t BitMapSlot;

#define BIT_MAP_MASK_ENTRY_SIZE (sizeof(BitMapSlot) * 8)

void bit_map_create(BitMap* map);
void bit_map_destroy(BitMap* map);

void bit_map_set_bit(BitMap* map, uint64_t slot, uint64_t bit, bool value);
bool bit_map_get_bit(BitMap* map, uint64_t slot, uint64_t bit);

BitMapSlot bit_map_get_slot(BitMap* map, uint64_t index);

void bit_map_set(BitMap* map, uint64_t index, bool value);
bool bit_map_get(BitMap* map, uint64_t index);

bool bit_map_find_first_unset(BitMap* map, uint64_t* index);

uint64_t bit_map_expand(BitMap* map);
uint64_t bit_map_compact(BitMap* map);

