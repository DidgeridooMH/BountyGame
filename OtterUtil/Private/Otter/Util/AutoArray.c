#include "Otter/Util/AutoArray.h"

#define ARRAY_INCREMENT_SIZE 32

void auto_array_create(AutoArray* array, uint32_t elementSize)
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

void* auto_array_allocate(AutoArray* array)
{
  if (array->size == array->capacity)
  {
    void* newBuffer =
        realloc(array->buffer, (array->capacity + ARRAY_INCREMENT_SIZE)
                                   * sizeof(array->sizeOfElement));
    if (newBuffer == NULL)
    {
      fprintf(stderr,
          "WARN: Unable to increase array size. Not allocating element.");
      return NULL;
    }
    array->buffer = newBuffer;
    array->capacity += ARRAY_INCREMENT_SIZE;
  }
  array->size += 1;
  return auto_array_get(array, array->size - 1);
}

void auto_array_clear(AutoArray* array)
{
  array->size = 0;
}
