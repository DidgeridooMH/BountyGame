#pragma once

#include "Otter/Util/export.h"

// TODO: Implement size optimization algorithm

typedef struct AutoArray
{
  uint32_t sizeOfElement;
  uint32_t size;
  uint32_t capacity;
  void* buffer;
} AutoArray;

OTTERUTIL_API void auto_array_create(AutoArray* array, uint32_t elementSize);

OTTERUTIL_API void auto_array_destroy(AutoArray* array);

OTTERUTIL_API void* auto_array_allocate(AutoArray* array);

OTTERUTIL_API void auto_array_clear(AutoArray* array);

OTTERUTIL_API inline void* auto_array_get(AutoArray* array, uint32_t index)
{
#ifdef _DEBUG
  if (index >= array->size)
  {
    fprintf(stderr, "WARN: Out of bounds read of index %d on array of size %d",
        index, array->size);
    return NULL;
  }
#endif
  return (char*) array->buffer + index * array->sizeOfElement;
}
