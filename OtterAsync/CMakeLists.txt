set(SOURCES
  Private/Otter/Async/Scheduler.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Async/export.h
  Public/Otter/Async/Scheduler.h
)

if (BUILD_STATIC)
  add_library(OtterAsync STATIC ${SOURCES} ${PUBLIC_HEADERS})
else()
  add_library(OtterAsync SHARED ${SOURCES} ${PUBLIC_HEADERS})
endif()

target_compile_definitions(OtterAsync PRIVATE OTTERASYNC_EXPORTS)
target_precompile_headers(OtterAsync PRIVATE Private/pch.h)
target_include_directories(OtterAsync PUBLIC Public)

add_custom_command(
  TARGET OtterAsync
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  $<TARGET_FILE:OtterAsync>
  ${CMAKE_SOURCE_DIR}/bin/tools/${CMAKE_BUILD_TYPE}/OtterAsync.dll
)

