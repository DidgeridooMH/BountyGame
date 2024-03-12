#pragma once

#include "Otter/Util/Json/Json.h"

JsonValue* json_parse_array_value(
    const char* document, size_t documentLength, size_t* const cursor);

void json_destroy_array(JsonValue* value);
