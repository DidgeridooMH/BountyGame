#pragma once

#include "Otter/Util/Json/Json.h"

char* json_parse_string(
    const char* document, size_t documentLength, size_t* const cursor);

JsonValue* json_parse_string_value(
    const char* document, size_t documentLength, size_t* const cursor);

void json_destroy_string_value(JsonValue* value);
