cmake_minimum_required(VERSION 3.8)
project(GameScript LANGUAGES CSharp)

set(SOURCES
  Source/Engine/IComponent.cs
  Source/Engine/Context.cs
  Source/Game/AdderComponent.cs
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
  "System"
)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY
    ${CMAKE_SOURCE_DIR}/../bin
)

if (BUILD_TESTS)
  add_subdirectory(Test)
endif()
