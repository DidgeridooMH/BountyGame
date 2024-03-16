#include "Otter/Util/StableAutoArray.h"

void stable_auto_array_create(
    StableAutoArray* array, uint32_t elementSize, uint32_t chunkSize)
{
  array->sizeOfElement = elementSize;
  array->chunkSize     = chunkSize;
  array->chunks        = NULL;
  array->capacity      = 0;
  array->size          = 0;
}

void stable_auto_array_destroy(StableAutoArray* array)
{
  while (array->chunks != NULL)
  {
    StableAutoArrayChunk* next = array->chunks->nextChunk;
    free(array->chunks);
    array->chunks = next;
  }
}

void* stable_auto_array_allocate(StableAutoArray* array)
{
  if (array->size == array->capacity)
  {
    StableAutoArrayChunk* newChunk = malloc(
        sizeof(StableAutoArrayChunk) + array->sizeOfElement * array->chunkSize);
    if (newChunk == NULL)
    {
      fprintf(stderr, "WARN: Out of memory. Not allocating element.\n");
      return NULL;
    }
    newChunk->nextChunk = NULL;

    StableAutoArrayChunk** chunkHead = &array->chunks;
    while (*chunkHead != NULL)
    {
      chunkHead = &(*chunkHead)->nextChunk;
    }
    *chunkHead = newChunk;

    array->capacity += array->chunkSize;
  }

  array->size += 1;
  return stable_auto_array_get(array, array->size - 1);
}

void stable_auto_array_clear(StableAutoArray* array)
{
  array->size = 0;
}
