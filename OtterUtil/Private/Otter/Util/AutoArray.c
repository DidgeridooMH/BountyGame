#include "Otter/Util/AutoArray.h"

#define ARRAY_INCREMENT_SIZE 32

void auto_array_create(AutoArray* array, size_t elementSize)
{
  array->buffer        = NULL;
  array->capacity      = 0;
  array->size          = 0;
  array->sizeOfElement = elementSize;
}

void auto_array_destroy(AutoArray* array)
{
  free(array->buffer);
}

static bool auto_array_expand(AutoArray* array, size_t requestedSize)
{
  size_t capacityOverrun = requestedSize % ARRAY_INCREMENT_SIZE;
  if (capacityOverrun > 0)
  {
    requestedSize += ARRAY_INCREMENT_SIZE - capacityOverrun;
  }

  void* newBuffer =
      realloc(array->buffer, requestedSize * array->sizeOfElement);
  if (newBuffer == NULL)
  {
    fprintf(stderr,
        "WARN: Unable to increase array size. Not allocating element.\n");
    return false;
  }
  array->buffer   = newBuffer;
  array->capacity = requestedSize;
  return true;
}

void* auto_array_allocate(AutoArray* array)
{
  if (array->size == array->capacity
      && !auto_array_expand(array, array->size + 1))
  {
  }
  array->size += 1;
  return auto_array_get(array, array->size - 1);
}

void* auto_array_allocate_many(AutoArray* array, size_t elementCount)
{
  size_t newSize = array->size + elementCount;
  if (newSize > array->capacity && !auto_array_expand(array, newSize))
  {
    return NULL;
  }
  array->size += elementCount;
  return auto_array_get(array, array->size - elementCount);
}

void auto_array_clear(AutoArray* array)
{
  array->size = 0;
}
