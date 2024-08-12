cmake_minimum_required(VERSION 3.12)
project(BountyGame LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (BUILD_TESTS)
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
  endif()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  enable_testing()
endif()

if (MSVC)
  add_compile_options(/D_UNICODE /DUNICODE /Zc:preprocessor)
elseif (WIN32)
  add_compile_options(-D_UNICODE -DUNICODE -Werror )
  add_compile_options(-ffile-prefix-map=${CMAKE_SOURCE_DIR}/=/)
endif()

add_subdirectory(OtterAsync)
add_subdirectory(OtterConfig)
add_subdirectory(OtterECS)
add_subdirectory(OtterMath)
add_subdirectory(OtterRender)
add_subdirectory(OtterScript)
add_subdirectory(OtterUtil)
add_subdirectory(Game)

set_target_properties(OtterAsync
  OtterConfig
  OtterECS
  OtterMath
  OtterScript
  OtterRender
  OtterUtil
  ${PROJECT_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE})

