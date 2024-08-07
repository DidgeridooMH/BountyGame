#pragma once

#include "Otter/Util/Array/StableAutoArray.h"
#include "Otter/Util/export.h"

OTTERUTIL_API void profiler_init(LARGE_INTEGER frequency);

OTTERUTIL_API void profiler_destroy();

OTTERUTIL_API void profiler_clock_start(const char* key);

OTTERUTIL_API void profiler_clock_end(const char* key);

OTTERUTIL_API float profiler_clock_get(const char* key);
