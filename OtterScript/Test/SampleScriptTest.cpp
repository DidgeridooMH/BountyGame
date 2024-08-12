#include "mono/metadata/object.h"
extern "C"
{
#include "Otter/Script/ScriptEngine.h"
#include "Otter/Util/Log.h"
}

int main()
{
  ScriptEngine engine;
  script_engine_init(&engine, "GameScriptTest.dll");

  LOG_DEBUG("Getting image");
  MonoImage* image = mono_assembly_get_image(engine.assembly);
  MonoClass* testClass =
      mono_class_from_name(image, "BountyGame", "SampleScript");
  if (!testClass)
  {
    LOG_ERROR("Unable to find class");
    return -1;
  }

  LOG_DEBUG("Getting interface");
  MonoClass* testInterface =
      mono_class_from_name(image, "BountyGame", "ISampleScript");
  if (!testInterface)
  {
    LOG_ERROR("Unable to find interface");
    return -1;
  }

  LOG_DEBUG("Creating object");
  MonoObject* testObject = mono_object_new(engine.domain, testClass);
  mono_runtime_object_init(testObject);

  MonoMethodDesc* methodDesc =
      mono_method_desc_new("BountyGame.ISampleScript:DoThing", false);
  MonoMethod* testMethod =
      mono_method_desc_search_in_class(methodDesc, testInterface);
  if (!testMethod)
  {
    LOG_ERROR("Unable to find method");
    return -1;
  }

  MonoMethod* derivedMethod =
      mono_object_get_virtual_method(testObject, testMethod);

  LOG_DEBUG("Invoking method");
  MonoObject* result =
      mono_runtime_invoke(derivedMethod, testObject, NULL, NULL);

  script_engine_shutdown(&engine);
  return 0;
}
