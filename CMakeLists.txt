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
add_subdirectory(OtterGameState)
add_subdirectory(OtterMath)
add_subdirectory(OtterNetworking)
add_subdirectory(OtterRender)
add_subdirectory(OtterUtil)
add_subdirectory(Client)
add_subdirectory(Server)

set_target_properties(OtterAsync
  OtterConfig
  OtterGameState
  OtterMath
  OtterNetworking
  OtterRender
  OtterUtil
  GameClient
  GameServer
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

