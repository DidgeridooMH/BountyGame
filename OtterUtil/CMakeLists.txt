set(SOURCES
  Private/Otter/Util/Json/Json.c
  Private/Otter/Util/Json/JsonArray.c
  Private/Otter/Util/Json/JsonObject.c
  Private/Otter/Util/AutoArray.c
  Private/Otter/Util/File.c
  Private/Otter/Util/Hash.c
  Private/Otter/Util/HashMap.c
  Private/Otter/Util/Profiler.c
  Private/Otter/Util/StableAutoArray.c
)

set(PRIVATE_HEADERS
  Private/Otter/Util/Json/JsonArray.h
  Private/Otter/Util/Json/JsonObject.h
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Util/Json/Json.h
  Public/Otter/Util/AutoArray.h
  Public/Otter/Util/File.h
  Public/Otter/Util/HashMap.h
  Public/Otter/Util/Profiler.h
  Public/Otter/Util/StableAutoArray.h
)

add_library(OtterUtil SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterUtil PRIVATE OTTERUTIL_EXPORTS)
target_precompile_headers(OtterUtil PRIVATE Private/pch.h)
target_include_directories(OtterUtil PUBLIC Public PRIVATE Private)

add_library(OtterUtil_static STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterUtil_static PUBLIC OTTERUTIL_STATIC)
target_precompile_headers(OtterUtil_static PRIVATE Private/pch.h)
target_include_directories(OtterUtil_static PUBLIC Public PRIVATE Private)

if (BUILD_TESTS)
  add_subdirectory(Test)
endif()

