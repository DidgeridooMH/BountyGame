#pragma once

#ifdef OTTERCONFIG_EXPORTS
#define OTTERCONFIG_API __declspec(dllexport)
#else
#define OTTERCONFIG_API __declspec(dllimport)
#endif
