set(SOURCES
  Private/Otter/ECS/ComponentPool.c
  Private/Otter/ECS/Entity.c
  Private/Otter/ECS/EntityComponentMap.c
  Private/Otter/ECS/SystemRegistry.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/ECS/export.h
  Public/Otter/ECS/ComponentPool.h
  Public/Otter/ECS/Entity.h
  Public/Otter/ECS/EntityComponentMap.h
  Public/Otter/ECS/SystemRegistry.h
)

add_library(OtterECS SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterECS PRIVATE OTTERECS_EXPORTS)
target_precompile_headers(OtterECS PRIVATE Private/pch.h)
target_include_directories(OtterECS PUBLIC Public PRIVATE Private)
target_link_libraries(OtterECS OtterUtil OtterMath OtterScript)

if (BUILD_TESTS)
  add_custom_command(
    TARGET OtterECS
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/OtterECS.dll
    ${CMAKE_SOURCE_DIR}/bin/test/${CMAKE_BUILD_TYPE}/OtterECS.dll
  )

  add_subdirectory(Test)
endif()

