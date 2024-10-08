set(SOURCES
  Private/Otter/Util/Array/AutoArray.c
  Private/Otter/Util/Array/SparseAutoArray.c
  Private/Otter/Util/Array/StableAutoArray.c
  Private/Otter/Util/Json/Json.c
  Private/Otter/Util/Json/JsonArray.c
  Private/Otter/Util/Json/JsonObject.c
  Private/Otter/Util/BitMap.c
  Private/Otter/Util/File.c
  Private/Otter/Util/Hash.c
  Private/Otter/Util/HashMap.c
  Private/Otter/Util/Heap.c
  Private/Otter/Util/Log.c
  Private/Otter/Util/Profiler.c
)

set(PRIVATE_HEADERS
  Private/Otter/Util/Json/JsonArray.h
  Private/Otter/Util/Json/JsonObject.h
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Util/Array/AutoArray.h
  Public/Otter/Util/Array/SparseAutoArray.h
  Public/Otter/Util/Array/StableAutoArray.h
  Public/Otter/Util/Json/Json.h
  Public/Otter/Util/BitMap.h
  Public/Otter/Util/File.h
  Public/Otter/Util/Hash.h
  Public/Otter/Util/HashMap.h
  Public/Otter/Util/Heap.h
  Public/Otter/Util/Log.h
  Public/Otter/Util/Profiler.h
)

add_library(OtterUtil SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterUtil PRIVATE OTTERUTIL_EXPORTS)
target_precompile_headers(OtterUtil PRIVATE Private/pch.h)
target_include_directories(OtterUtil PUBLIC Public PRIVATE Private)

if (BUILD_TESTS)
  add_custom_command(
    TARGET OtterUtil
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:OtterUtil>
    ${CMAKE_SOURCE_DIR}/bin/test/${CMAKE_BUILD_TYPE}/OtterUtil.dll
  )

  add_subdirectory(Test)
endif()

