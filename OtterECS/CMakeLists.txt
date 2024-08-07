set(SOURCES
  Private/Otter/ECS/ComponentPool.c
  Private/Otter/ECS/EntityComponentMap.c
  Private/Otter/ECS/SystemRegistry.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/ECS/export.h
  Public/Otter/ECS/ComponentPool.h
  Public/Otter/ECS/EntityComponentMap.h
  Public/Otter/ECS/SystemRegistry.h
)

add_library(OtterECS SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterECS PRIVATE OTTERECS_EXPORTS)
target_precompile_headers(OtterECS PRIVATE Private/pch.h)
target_include_directories(OtterECS PUBLIC Public PRIVATE Private)
target_link_libraries(OtterECS OtterUtil)

if (BUILD_TESTS)
  add_library(OtterECS_static STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
  target_compile_definitions(OtterECS_static PUBLIC OTTERECS_STATIC)
  target_precompile_headers(OtterECS_static PRIVATE Private/pch.h)
  target_include_directories(OtterECS_static PUBLIC Public PRIVATE Private)
  target_link_libraries(OtterECS_static OtterUtil_static)

  add_subdirectory(Test)
endif()

