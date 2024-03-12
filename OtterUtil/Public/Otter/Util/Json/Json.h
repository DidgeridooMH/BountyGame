#pragma once

#include "Otter/Util/AutoArray.h"
#include "Otter/Util/HashMap.h"
#include "Otter/Util/export.h"

enum JsonType
{
  JT_OBJECT,
  JT_ARRAY,
  JT_STRING,
  JT_NUMBER,
  JT_BOOLEAN,
};

typedef struct JsonValue
{
  enum JsonType type;
  union
  {
    HashMap* object;
    AutoArray array;
    char* string;
    float number;
    bool boolean;
  };
} JsonValue;

OTTERUTIL_API JsonValue* json_parse(
    const char* document, size_t documentLength, size_t* const cursor);

OTTERUTIL_API void json_destroy(JsonValue* value);
