#pragma once

#ifdef OTTERASYNC_EXPORTS
#define OTTERASYNC_API __declspec(dllexport)
#else
#define OTTERASYNC_API __declspec(dllimport)
#endif
