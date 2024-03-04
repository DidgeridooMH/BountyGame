#pragma once

#ifdef OTTERRENDER_EXPORTS
#define OTTERRENDER_API __declspec(dllexport)
#else
#define OTTERRENDER_API __declspec(dllimport)
#endif
