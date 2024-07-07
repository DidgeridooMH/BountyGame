#include "Otter/Util/Log.h"

#include <time.h>

static void log_set_foreground_color(int color)
{
  printf("\x1b[38;5;%dm", color);
}

static void log_reset_foreground_color()
{
  printf("\x1b[0m");
}

void log_message(int sourceLine, const char* sourceFile, LogVerbosity verbosity,
    const char* message, ...)
{
  va_list args;
  va_start(args, message);

  time_t now = time(NULL);
  struct tm timeinfo;
  localtime_s(&timeinfo, &now);
  printf("%02d:%02d:%02d ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  switch (verbosity)
  {
  case LOG_DEBUG:
    log_set_foreground_color(14);
    printf("DEBUG ");
    break;
  case LOG_WARNING:
    log_set_foreground_color(11);
    printf("WARN  ");
    break;
  case LOG_ERROR:
    log_set_foreground_color(9);
    printf("ERROR ");
    break;
  default:
    log_set_foreground_color(5);
    printf("UKNWN ");
    break;
  }
  log_set_foreground_color(249);
  printf("%s:%d: ", sourceFile, sourceLine);
  log_reset_foreground_color();
  vprintf(message, args);
  printf("\n");

  va_end(args);
}
