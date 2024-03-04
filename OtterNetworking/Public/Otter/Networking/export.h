#pragma once

#ifdef OTTERNETWORKING_EXPORTS
#define OTTERNETWORKING_API __declspec(dllexport)
#else
#define OTTERNETWORKING_API __declspec(dllimport)
#endif
