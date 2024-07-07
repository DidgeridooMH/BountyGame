#pragma once

#include "Otter/Util/export.h"

#define LOG_MSG(verbosity, message, ...) \
  log_message(__LINE__, __FILE__, verbosity, message, ##__VA_ARGS__)

// #ifdef _DEBUG
#define LOG_DEBUG(message, ...) LOG_MSG(LOG_DEBUG, message, ##__VA_ARGS__)
// #else
// #define LOG_DEBUG(message, ...)
// #endif
#define LOG_WARNING(message, ...) LOG_MSG(LOG_WARNING, message, ##__VA_ARGS__)
#define LOG_ERROR(message, ...)   LOG_MSG(LOG_ERROR, message, ##__VA_ARGS__)

typedef enum LogVerbosity
{
  LOG_DEBUG,
  LOG_WARNING,
  LOG_ERROR
} LogVerbosity;

OTTERUTIL_API void log_message(int sourceLine, const char* sourceFile,
    LogVerbosity verbosity, const char* message, ...);
