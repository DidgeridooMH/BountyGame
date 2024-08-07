#pragma once

#include <stdint.h>

#include "Otter/Util/Array/AutoArray.h"
#include "Otter/Util/export.h"

typedef AutoArray BitMap;
typedef uint64_t BitMapSlot;

#define BIT_MAP_MASK_ENTRY_SIZE (sizeof(BitMapSlot) * 8)

OTTERUTIL_API void bit_map_create(BitMap* map);
OTTERUTIL_API void bit_map_destroy(BitMap* map);

OTTERUTIL_API void bit_map_set_bit(
    BitMap* map, uint64_t slot, uint64_t bit, bool value);
OTTERUTIL_API bool bit_map_get_bit(BitMap* map, uint64_t slot, uint64_t bit);

OTTERUTIL_API BitMapSlot bit_map_get_slot(BitMap* map, uint64_t index);

OTTERUTIL_API void bit_map_set(BitMap* map, uint64_t index, bool value);
OTTERUTIL_API bool bit_map_get(BitMap* map, uint64_t index);

OTTERUTIL_API bool bit_map_find_first_unset(BitMap* map, uint64_t* index);

OTTERUTIL_API uint64_t bit_map_expand(BitMap* map);
OTTERUTIL_API uint64_t bit_map_compact(BitMap* map);

