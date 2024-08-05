set(SOURCE
  Source/Config/GameConfig.c
  Source/Input/InputMap.c
  Source/Window/GameWindow.c
  Source/Main.c
)

set(HEADERS
  Source/Config/GameConfig.h
  Source/Input/InputMap.h
  Source/Window/GameWindow.h
  Source/pch.h
)

find_package(Vulkan REQUIRED)

add_executable(${PROJECT_NAME} ${SOURCE} ${HEADERS})
target_include_directories(${PROJECT_NAME} PRIVATE Source)
target_link_libraries(${PROJECT_NAME} PRIVATE
  OtterRender
  OtterAsync
  OtterECS
  OtterMath
  OtterConfig
  OtterUtil
  Vulkan::Vulkan)
target_precompile_headers(${PROJECT_NAME} PRIVATE Source/pch.h)

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE shcore.lib xinput.lib)
endif()
