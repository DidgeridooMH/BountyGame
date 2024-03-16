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
    HashMap object;
    AutoArray array;
    char* string;
    float number;
    bool boolean;
  };
} JsonValue;

enum JsonTokenType
{
  JTT_ERROR,
  JTT_LDRAGON,
  JTT_RDRAGON,
  JTT_LBRACKET,
  JTT_RBRACKET,
  JTT_COLON,
  JTT_COMMA,
  JTT_STRING,
  JTT_NUMBER,
  JTT_TRUE,
  JTT_FALSE,
  JTT_NULL
};

typedef struct JsonToken
{
  enum JsonTokenType type;
  union
  {
    struct
    {
      const char* tokenString;
      size_t tokenStringLength;
    };
    float tokenFloat;
  };
} JsonToken;

bool json_get_token(JsonToken* token, const char* document,
    size_t documentLength, size_t* const cursor);

void json_peek_token(JsonToken* token, const char* document,
    size_t documentLength, size_t cursor);

OTTERUTIL_API JsonValue* json_parse(
    const char* document, size_t documentLength, size_t* const cursor);

OTTERUTIL_API void json_destroy(JsonValue* value);
