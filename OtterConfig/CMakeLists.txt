set(SOURCES
  Private/Otter/Config/Config.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Config/Config.h
  Public/Otter/Config/export.h
)

if (BUILD_STATIC)
  add_library(OtterConfig STATIC ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
else()
  add_library(OtterConfig SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
endif()

target_compile_definitions(OtterConfig PRIVATE OTTERCONFIG_EXPORTS)
target_precompile_headers(OtterConfig PRIVATE Private/pch.h)
target_include_directories(OtterConfig PUBLIC Public PRIVATE Private)
target_link_libraries(OtterConfig PRIVATE OtterUtil)

