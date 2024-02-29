#pragma once

#ifdef OTTERRENDER_EXPORTS
#define OTTER_API __declspec(dllexport)
#else
#define OTTER_API __declspec(dllimport)
#endif
