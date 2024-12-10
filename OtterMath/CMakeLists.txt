set(SOURCES
  Private/Otter/Math/Clamp.c
  Private/Otter/Math/Mat.c
  Private/Otter/Math/Projection.c
  Private/Otter/Math/Transform.c
  Private/Otter/Math/Vec.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Math/Clamp.h
  Public/Otter/Math/export.h
  Public/Otter/Math/Mat.h
  Public/Otter/Math/MatDef.h
  Public/Otter/Math/Projection.h
  Public/Otter/Math/Transform.h
  Public/Otter/Math/Vec.h
)

add_library(OtterMath SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterMath PRIVATE OTTERMATH_EXPORTS)
target_precompile_headers(OtterMath PRIVATE Private/pch.h)
target_include_directories(OtterMath PUBLIC Public PRIVATE Private)

if (BUILD_TESTS)
  add_custom_command(
    TARGET OtterMath
    POST_BUILD
    COMMAND
      ${CMAKE_COMMAND} -E copy
      $<TARGET_FILE:OtterMath>
      ${CMAKE_SOURCE_DIR}/bin/test/${CMAKE_BUILD_TYPE}/OtterMath.dll
  )
endif()

add_custom_command(
  TARGET OtterMath
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  $<TARGET_FILE:OtterMath>
  ${CMAKE_SOURCE_DIR}/bin/tools/${CMAKE_BUILD_TYPE}/OtterMath.dll
)

