set(SOURCE
  Source/Input/Input.c
  Source/Window/GameWindow.c
  Source/Client.c
)

set(HEADERS
  Source/Input/Input.h
  Source/Window/GameWindow.h
  Source/Client.h
  Source/pch.h
)

find_package(Vulkan REQUIRED)

add_executable(GameClient ${SOURCE} ${HEADERS})
target_include_directories(GameClient PRIVATE Source)
target_link_libraries(GameClient PRIVATE
  OtterRender
  OtterGameState
  OtterAsync
  OtterMath
  OtterNetworking
  OtterConfig
  OtterUtil
  Vulkan::Vulkan)
target_precompile_headers(GameClient PRIVATE Source/pch.h)

if (WIN32)
  target_link_libraries(GameClient PRIVATE shcore.lib)
endif()
