#include "Otter/Util/Json/JsonString.h"

char* json_parse_string(
    const char* document, size_t documentLength, size_t* const cursor)
{
  *cursor += 1;
  size_t startOfString = *cursor;
  while (*cursor < documentLength && document[*cursor] != '\"')
  {
    *cursor += 1;
  }
  if (document[*cursor] != '\"')
  {
    fprintf(stderr, "ERR: Unable to parse json.\n");
    return NULL;
  }
  char* stringValue = malloc((size_t) *cursor - startOfString + 1);
  if (stringValue == NULL)
  {
    fprintf(stderr, "ERR: OOM\n");
    return NULL;
  }
  strncpy_s(stringValue, (size_t) *cursor - startOfString + 1,
      &document[startOfString], (size_t) *cursor - startOfString);
  *cursor += 1;
  return stringValue;
}

JsonValue* json_parse_string_value(
    const char* document, size_t documentLength, size_t* const cursor)
{
  char* stringValue = json_parse_string(document, documentLength, cursor);
  if (stringValue == NULL)
  {
    return NULL;
  }

  JsonValue* jsonString = malloc(sizeof(JsonValue));
  if (jsonString == NULL)
  {
    fprintf(stderr, "ERR: OOM\n");
    free(stringValue);
    return NULL;
  }
  jsonString->type   = JT_STRING;
  jsonString->string = stringValue;
  return jsonString;
}

void json_destroy_string_value(JsonValue* value)
{
  free(value->string);
}
