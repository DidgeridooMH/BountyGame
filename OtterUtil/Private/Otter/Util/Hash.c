#include "Otter/Util/Hash.h"

size_t hash_key(const void* key, size_t keyLength, size_t coefficient)
{
  size_t hash = 0;
  for (size_t i = 0; i < keyLength; ++i)
  {
    hash = ((hash << 2) + *(char*) key) * coefficient;
    key++;
  }
  return hash;
}
