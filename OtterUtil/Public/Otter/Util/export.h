#pragma once

#ifdef OTTERUTIL_EXPORTS
#define OTTERUTIL_API __declspec(dllexport)
#else
#define OTTERUTIL_API __declspec(dllimport)
#endif

