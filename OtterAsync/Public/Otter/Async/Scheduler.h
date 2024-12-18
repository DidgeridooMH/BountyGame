#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Otter/Async/export.h"

// TODO: Get this shit outta here and make a proper task graph for better
// performance.

#define TASK_SCHEDULER_THREADS 16

enum TaskFlags
{
  TASK_FLAGS_FREE_DATA_ON_COMPLETE = 0b1
};

typedef void (*TaskFunction)(void* userData, int threadId);

OTTERASYNC_API void task_scheduler_init();

OTTERASYNC_API void task_scheduler_destroy();

OTTERASYNC_API int task_scheduler_get_number_of_threads();

OTTERASYNC_API HANDLE task_scheduler_enqueue(
    TaskFunction function, void* data, enum TaskFlags flags);
