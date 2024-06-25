cmake_minimum_required(VERSION 3.12)
project(BountyGame)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

if (MSVC)
  add_compile_options(/D_UNICODE /DUNICODE)
elseif (WIN32)
  add_compile_options(-D_UNICODE -DUNICODE -Werror)
endif()

add_subdirectory(OtterAsync)
add_subdirectory(OtterConfig)
add_subdirectory(OtterMath)
add_subdirectory(OtterRender)
add_subdirectory(OtterUtil)
add_subdirectory(Game)

set_target_properties(OtterAsync
  OtterConfig
  OtterMath
  OtterRender
  OtterUtil
  ${PROJECT_NAME}
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

