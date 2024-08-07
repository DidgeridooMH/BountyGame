#pragma once

#include "Otter/Util/Array/StableAutoArray.h"
#include "Otter/Util/Hash.h"
#include "Otter/Util/export.h"

#define HASH_MAP_DEFAULT_BUCKETS 512
#define HASH_MAP_DEFAULT_COEF    769

typedef struct KeyValue
{
  Key key;
  union
  {
    void* ptrValue;
    float floatValue;
  };
} KeyValue;

typedef struct HashMap
{
  StableAutoArray* buckets;
  size_t numOfBuckets;
  size_t coefficient;
} HashMap;

typedef void (*HashMapDestroyFn)(void*);
typedef void (*HashMapIterateFn)(void*, size_t, void*, void*);

OTTERUTIL_API bool hash_map_create(
    HashMap* map, size_t numOfBuckets, size_t coefficient);

OTTERUTIL_API void hash_map_destroy(HashMap* map, HashMapDestroyFn destructor);

OTTERUTIL_API bool hash_map_set_value(
    HashMap* map, const void* key, size_t keyLength, void* value);

OTTERUTIL_API void* hash_map_get_value(
    HashMap* map, const void* key, size_t keyLength);

OTTERUTIL_API bool hash_map_set_value_float(
    HashMap* map, const void* key, size_t keyLength, float value);

OTTERUTIL_API float hash_map_get_value_float(
    HashMap* map, const void* key, size_t keyLength);

OTTERUTIL_API void hash_map_iterate(
    HashMap* map, HashMapIterateFn iterator, void* userData);
