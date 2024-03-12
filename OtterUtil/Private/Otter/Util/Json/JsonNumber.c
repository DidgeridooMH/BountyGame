#include "Otter/Util/Json/JsonNumber.h"

JsonValue* json_parse_number_value(
    const char* document, size_t documentLength, size_t* const cursor)
{
  char* endOfNumber;
  float numberValue = strtof(&document[*cursor], &endOfNumber);
  *cursor += (endOfNumber - &document[*cursor]);

  JsonValue* jsonNumber = malloc(sizeof(JsonValue));
  if (jsonNumber == NULL)
  {
    fprintf(stderr, "ERR: OOM\n");
    return NULL;
  }
  jsonNumber->type   = JT_NUMBER;
  jsonNumber->number = numberValue;
  return jsonNumber;
}
