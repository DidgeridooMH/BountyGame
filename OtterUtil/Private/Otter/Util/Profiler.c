#include "Otter/Util/Profiler.h"

#include "Otter/Util/HashMap.h"
#include "Otter/Util/Log.h"

#define PROFILE_TIME_SAMPLE_COUNT 50

typedef struct ProfileTime
{
  uint32_t cursor;
  uint32_t numOfSamples;
  float times[PROFILE_TIME_SAMPLE_COUNT];
  float totalTime;
  LARGE_INTEGER startTime;
} ProfileTime;

static HashMap g_clockTimes;
static bool g_profilingEnabled;
static LARGE_INTEGER g_timerFrequency;

void profiler_init(LARGE_INTEGER frequency)
{
  if (!hash_map_create(
          &g_clockTimes, HASH_MAP_DEFAULT_BUCKETS, HASH_MAP_DEFAULT_COEF))
  {
    LOG_WARNING("Profiler did not initialize.");
    return;
  }

  g_profilingEnabled = true;
  g_timerFrequency   = frequency;
}

void profiler_destroy()
{
  hash_map_destroy(&g_clockTimes, free);
  g_profilingEnabled = false;
}

void profiler_clock_start(const char* key)
{
  ProfileTime* profileTime =
      hash_map_get_value(&g_clockTimes, key, strlen(key));
  if (profileTime == NULL)
  {
    profileTime = calloc(1, sizeof(ProfileTime));
    if (profileTime == NULL
        || !hash_map_set_value(&g_clockTimes, key, strlen(key), profileTime))
    {
      return;
    }
  }
  QueryPerformanceCounter(&profileTime->startTime);
}

void profiler_clock_end(const char* key)
{
  ProfileTime* profileTime =
      hash_map_get_value(&g_clockTimes, key, strlen(key));
  if (profileTime == NULL)
  {
    return;
  }
  LARGE_INTEGER endTime;
  QueryPerformanceCounter(&endTime);
  profileTime->totalTime -= profileTime->times[profileTime->cursor];
  profileTime->times[profileTime->cursor] =
      (float) (endTime.QuadPart - profileTime->startTime.QuadPart)
      / g_timerFrequency.QuadPart;
  profileTime->totalTime += profileTime->times[profileTime->cursor];
  profileTime->cursor = (profileTime->cursor + 1) % PROFILE_TIME_SAMPLE_COUNT;
  profileTime->numOfSamples =
      (profileTime->numOfSamples + 1) % PROFILE_TIME_SAMPLE_COUNT;
}

float profiler_clock_get(const char* key)
{
  ProfileTime* profileTime =
      hash_map_get_value(&g_clockTimes, key, strlen(key));
  if (profileTime == NULL)
  {
    return INFINITY;
  }
  return profileTime->totalTime / profileTime->numOfSamples;
}
