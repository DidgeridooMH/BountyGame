set(TEST_SOURCES
  ISampleScript.cs
  SampleScript.cs
)

add_library(${PROJECT_NAME}Test SHARED ${TEST_SOURCES})

set_property(TARGET ${PROJECT_NAME}Test PROPERTY VS_DOTNET_REFERENCES
  "System"
)

set_target_properties(
  ${PROJECT_NAME}Test
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/../bin/test/
)
