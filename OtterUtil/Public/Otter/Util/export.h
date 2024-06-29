#pragma once

#ifdef OTTERUTIL_EXPORTS
#define OTTERUTIL_API __declspec(dllexport)
#elif defined(OTTERUTIL_STATIC)
#define OTTERUTIL_API
#else
#define OTTERUTIL_API __declspec(dllimport)
#endif
