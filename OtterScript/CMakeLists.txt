set(SOURCES
  Private/Otter/Script/ScriptEngine.c
)

set(PRIVATE_HEADERS
  Private/pch.h
)

set(PUBLIC_HEADERS
  Public/Otter/Script/export.h
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(Mono REQUIRED mono-2)

add_library(OtterScript SHARED ${SOURCES} ${PUBLIC_HEADERS} ${PRIVATE_HEADERS})
target_compile_definitions(OtterScript PRIVATE OTTERSCRIPT_EXPORTS)
target_precompile_headers(OtterScript PRIVATE Private/pch.h)
target_include_directories(OtterScript PUBLIC Public ${Mono_INCLUDE_DIRS} PRIVATE Private)
target_link_directories(OtterScript PUBLIC ${Mono_LIBRARY_DIRS})
target_link_libraries(OtterScript PUBLIC mono-2.0-sgen OtterUtil)

if (BUILD_TESTS)
  add_custom_command(
    TARGET OtterScript
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/OtterScript.dll
    ${CMAKE_SOURCE_DIR}/bin/test/${CMAKE_BUILD_TYPE}/OtterScript.dll
  )

  add_subdirectory(Test)
endif()

