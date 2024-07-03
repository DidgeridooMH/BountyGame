#include "Otter/Util/HashMap.h"

#include <math.h>

bool hash_map_create(HashMap* map, size_t numOfBuckets, size_t coefficient)
{
  map->numOfBuckets = numOfBuckets;
  map->coefficient  = coefficient;

  map->buckets = malloc(numOfBuckets * sizeof(StableAutoArray));
  if (map->buckets == NULL)
  {
    return false;
  }

  for (int i = 0; i < numOfBuckets; i++)
  {
    stable_auto_array_create(
        &map->buckets[i], sizeof(KeyValue), SAA_DEFAULT_CHUNK_SIZE);
  }

  return true;
}

void hash_map_destroy(HashMap* map, HashMapDestroyFn destructor)
{
  for (size_t i = 0; i < map->numOfBuckets; i++)
  {
    for (uint32_t e = 0; e < map->buckets[i].size; e++)
    {
      KeyValue* keyValue = stable_auto_array_get(&map->buckets[i], e);
      free(keyValue->key.key);

      if (destructor != NULL)
      {
        destructor(keyValue->ptrValue);
      }
    }

    stable_auto_array_destroy(&map->buckets[i]);
  }
  free(map->buckets);
}

static StableAutoArray* hash_map_get_bucket(
    HashMap* map, const void* key, size_t keyLength)
{
  size_t hash = hash_key(key, keyLength, map->coefficient);
  return &map->buckets[hash % map->numOfBuckets];
}

static KeyValue* hash_map_get_key_value(
    HashMap* map, const void* key, size_t keyLength)
{
  StableAutoArray* bucket = hash_map_get_bucket(map, key, keyLength);
  for (uint32_t i = 0; i < bucket->size; i++)
  {
    KeyValue* keyValue = stable_auto_array_get(bucket, i);
    if (keyLength == keyValue->key.keyLength
        && memcmp(keyValue->key.key, key, keyLength) == 0)
    {
      return keyValue;
    }
  }
  return NULL;
}

bool hash_map_set_value(
    HashMap* map, const void* key, size_t keyLength, void* value)
{
  KeyValue* keyValue = hash_map_get_key_value(map, key, keyLength);
  if (keyValue != NULL)
  {
    keyValue->ptrValue = value;
    return true;
  }

  StableAutoArray* bucket = hash_map_get_bucket(map, key, keyLength);
  keyValue                = stable_auto_array_allocate(bucket);
  if (keyValue == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  keyValue->key.key = malloc(keyLength);
  if (keyValue->key.key == NULL)
  {
    return false;
  }
  memcpy(keyValue->key.key, key, keyLength);
  keyValue->key.keyLength = keyLength;

  keyValue->ptrValue = value;

  return true;
}

void* hash_map_get_value(HashMap* map, const void* key, size_t keyLength)
{
  KeyValue* keyValue = hash_map_get_key_value(map, key, keyLength);
  if (keyValue != NULL)
  {
    return keyValue->ptrValue;
  }
  return NULL;
}

bool hash_map_set_value_float(
    HashMap* map, const void* key, size_t keyLength, float value)
{
  KeyValue* keyValue = hash_map_get_key_value(map, key, keyLength);
  if (keyValue != NULL)
  {
    keyValue->floatValue = value;
    return true;
  }

  StableAutoArray* bucket = hash_map_get_bucket(map, key, keyLength);
  keyValue                = stable_auto_array_allocate(bucket);
  if (keyValue == NULL)
  {
    fprintf(stderr, "OOM\n");
    return false;
  }

  keyValue->key.key = malloc(keyLength);
  if (keyValue->key.key == NULL)
  {
    return false;
  }
  memcpy(keyValue->key.key, key, keyLength);
  keyValue->key.keyLength = keyLength;

  keyValue->floatValue = value;

  return true;
}

float hash_map_get_value_float(HashMap* map, const void* key, size_t keyLength)
{
  KeyValue* keyValue = hash_map_get_key_value(map, key, keyLength);
  if (keyValue != NULL)
  {
    return keyValue->floatValue;
  }
  return NAN;
}
