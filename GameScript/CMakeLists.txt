cmake_minimum_required(VERSION 3.8)
project(GameScript LANGUAGES CSharp)

set(SOURCES
  Source/Engine/IComponent.cs
  Source/Engine/BaseComponent.cs
  Source/Engine/Context.cs
  Source/Engine/ECS/ComponentType.cs
  Source/Engine/ECS/EntityComponentMap.cs
  Source/Engine/ECS/Entity.cs
  Source/Engine/Math/Transform.cs
  Source/Engine/Math/Vec.cs
  Source/Game/OscillatePositionComponent.cs
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
