#pragma once

#ifdef OTTERMATH_EXPORTS
#define OTTERMATH_API __declspec(dllexport)
#else
#define OTTERMATH_API __declspec(dllimport)
#endif
