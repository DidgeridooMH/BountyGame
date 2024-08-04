#pragma once

#ifdef OTTERECS_EXPORTS
#define OTTERECS_API __declspec(dllexport)
#elif defined(OTTERECS_STATIC)
#define OTTERECS_API
#else
#define OTTERECS_API __declspec(dllimport)
#endif

