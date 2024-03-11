#pragma once

#ifdef OTTERGAMESTATE_EXPORTS
#define OTTERGAMESTATE_API __declspec(dllexport)
#else
#define OTTERGAMESTATE_API __declspec(dllimport)
#endif
