#pragma once

#include <stdint.h>

#include "Otter/Util/Array/AutoArray.h"
#include "Otter/Util/BitMap.h"
#include "Otter/Util/export.h"

#define COMPONENT_ID_INVALID ~0ULL

typedef struct SparseAutoArray
{
  BitMap usedMask;
  AutoArray components;
} SparseAutoArray;

/** @brief Create a sparse auto array. */
OTTERUTIL_API void sparse_auto_array_create(
    SparseAutoArray* list, uint64_t componentSize);

/** @brief Destroy a sparse auto array. */
OTTERUTIL_API void sparse_auto_array_destroy(SparseAutoArray* list);

/**
 * @brief Allocate a new element in the sparse auto array.
 *
 * @param list The sparse auto array.
 * @return The index of the allocated element.
 */
OTTERUTIL_API uint64_t sparse_auto_array_allocate(SparseAutoArray* list);

/**
 * @brief Deallocate an element in the sparse auto array.
 *
 * @param list The sparse auto array.
 * @param index The index of the element to deallocate.
 * @return The number of compacted entries.
 */
OTTERUTIL_API uint64_t sparse_auto_array_deallocate(
    SparseAutoArray* list, uint64_t index);

/**
 * @brief Get an element in the sparse auto array.
 *
 * @param list The sparse auto array.
 * @param index The index of the element to get.
 * @return A pointer to the element.
 */
OTTERUTIL_API void* sparse_auto_array_get(
    SparseAutoArray* list, uint64_t index);

