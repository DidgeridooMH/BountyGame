#pragma once

#include "Otter/Util/export.h"

typedef struct Key
{
  char* key;
  size_t keyLength;
} Key;

OTTERUTIL_API size_t hash_key(
    const void* key, size_t keyLength, size_t coefficient);

