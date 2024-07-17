#pragma once

#include "Otter/Util/export.h"

OTTERUTIL_API char* file_load(const char* path, uint64_t* fileLength);

OTTERUTIL_API void file_write(
    const char* path, const char* data, uint64_t length);
