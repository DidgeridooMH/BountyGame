#pragma once

#include "Otter/Util/AutoArray.h"
#include "Otter/Util/export.h"

/** @brief A 32 bit int to float key-pair. */
typedef struct HeapElement
{
  uint32_t key;
  float value;
} HeapElement;

/** @brief A heap of key-value pairs. */
typedef struct Heap
{
  AutoArray contents;
} Heap;

/** @brief Create a new heap. */
OTTERUTIL_API void heap_create(Heap* heap);

/** @brief Push a new key-value pair onto the heap. */
OTTERUTIL_API void heap_push(Heap* heap, uint32_t key, float value);

/** @brief Pop the top key-value pair from the heap. */
OTTERUTIL_API void heap_pop(Heap* heap);

/** @brief Get the top key-value pair from the heap without removing it. */
OTTERUTIL_API void heap_top(Heap* heap, uint32_t* key, float* value);

/** @brief Destroy the heap. */
OTTERUTIL_API void heap_destroy(Heap* heap);
