#pragma once

#include "Otter/Util/Json/Json.h"

JsonValue* json_parse_number_value(
    const char* document, size_t documentLength, size_t* const cursor);
