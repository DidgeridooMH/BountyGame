set(SOURCE
  SampleScriptTest.cpp
)

add_executable(ScriptTest ${SOURCE})
target_link_libraries(ScriptTest OtterScript)

set_target_properties(
  ScriptTest
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY
  ${CMAKE_SOURCE_DIR}/bin/test/${CMAKE_BUILD_TYPE}
)

