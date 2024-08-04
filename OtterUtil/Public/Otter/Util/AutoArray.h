#pragma once

#include <stdlib.h>

#include "Otter/Util/Log.h"
#include "Otter/Util/export.h"

typedef struct AutoArray
{
  size_t sizeOfElement;
  size_t size;
  size_t capacity;
  void* buffer;
} AutoArray;

OTTERUTIL_API void auto_array_create(AutoArray* array, size_t elementSize);

OTTERUTIL_API void auto_array_destroy(AutoArray* array);

OTTERUTIL_API void* auto_array_allocate(AutoArray* array);

OTTERUTIL_API void* auto_array_allocate_many(
    AutoArray* array, size_t elementCount);

OTTERUTIL_API void auto_array_clear(AutoArray* array);

OTTERUTIL_API inline void* auto_array_get(AutoArray* array, size_t index)
{
#ifdef _DEBUG
  if (index >= array->size)
  {
    LOG_WARNING("Out of bounds read of index %zd on array of size %zd", index,
        array->size);
    return NULL;
  }
#endif
  return (char*) array->buffer + index * array->sizeOfElement;
}

OTTERUTIL_API void auto_array_pop(AutoArray* array);

OTTERUTIL_API void auto_array_pop_many(AutoArray* array, size_t elementCount);
