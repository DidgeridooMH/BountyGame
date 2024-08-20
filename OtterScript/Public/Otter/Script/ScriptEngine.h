#pragma once

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/debug-helpers.h>

#include "Otter/Script/export.h"

#define ENGINE_NAMESPACE      "OtterEngine"
#define GAME_SCRIPT_NAMESPACE "BountyGame"

typedef struct ScriptEngine
{
  MonoDomain* domain;
  MonoAssembly* assembly;
} ScriptEngine;

OTTERSCRIPT_API void script_engine_init(
    ScriptEngine* engine, const char* gameAssemblyName);

OTTERSCRIPT_API void script_engine_shutdown(ScriptEngine* engine);

OTTERSCRIPT_API bool script_engine_create_component(ScriptEngine* engine,
    const char* componentName, uint32_t* component, uint64_t entity);

OTTERSCRIPT_API void script_engine_destroy_component(
    ScriptEngine* engine, uint32_t component);

OTTERSCRIPT_API void script_engine_run_update(
    ScriptEngine* scriptEngine, uint32_t scriptComponent, void* context);

