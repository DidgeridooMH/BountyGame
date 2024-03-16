#pragma once

#include "Otter/Util/export.h"

// TODO: Implement size optimization algorithm

#define SAA_DEFAULT_CHUNK_SIZE 32

typedef struct StableAutoArrayChunk
{
  struct StableAutoArrayChunk* nextChunk;
  char buffer[];
} StableAutoArrayChunk;

typedef struct StableAutoArray
{
  uint32_t sizeOfElement;
  uint32_t chunkSize;
  uint32_t size;
  uint32_t capacity;
  StableAutoArrayChunk* chunks;
} StableAutoArray;

OTTERUTIL_API void stable_auto_array_create(
    StableAutoArray* array, uint32_t elementSize, uint32_t chunkSize);

OTTERUTIL_API void stable_auto_array_destroy(StableAutoArray* array);

OTTERUTIL_API void* stable_auto_array_allocate(StableAutoArray* array);

OTTERUTIL_API void stable_auto_array_clear(StableAutoArray* array);

OTTERUTIL_API inline void* stable_auto_array_get(
    StableAutoArray* array, uint32_t index)
{
#ifdef _DEBUG
  if (index >= array->size)
  {
    fprintf(stderr, "WARN: Out of bounds read of index %d on array of size %d",
        index, array->size);
    return NULL;
  }
#endif

  StableAutoArrayChunk* chunk = array->chunks;
  for (uint32_t i = 0; i < index / array->chunkSize; i++)
  {
    chunk = chunk->nextChunk;
  }

  return (char*) chunk->buffer
       + (index % array->chunkSize) * array->sizeOfElement;
}
