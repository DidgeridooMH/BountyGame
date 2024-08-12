#pragma once

#ifdef OTTERSCRIPT_EXPORTS
#define OTTERSCRIPT_API __declspec(dllexport)
#else
#define OTTERSCRIPT_API __declspec(dllimport)
#endif

