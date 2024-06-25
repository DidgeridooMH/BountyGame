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

if (BUILD_STATIC)
  add_library(OtterMath STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
else()
  add_library(OtterMath SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
endif()

target_compile_definitions(OtterMath PRIVATE OTTERMATH_EXPORTS)
target_precompile_headers(OtterMath PRIVATE Private/pch.h)
target_include_directories(OtterMath PUBLIC Public PRIVATE Private)

