set(SOURCE
  Source/Config/GameConfig.c
  Source/Input/InputMap.c
  Source/Render/RenderSystem.c
  Source/Window/GameWindow.c
  Source/FreeCameraControls.c
  Source/Main.c
)

set(HEADERS
  Source/Components/ComponentType.h
  Source/Config/GameConfig.h
  Source/Input/InputMap.h
  Source/Render/RenderSystem.h
  Source/Window/GameWindow.h
  Source/FreeCameraControls.h
  Source/Context.h
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
  OtterScript
  OtterUtil
  Vulkan::Vulkan)
target_precompile_headers(${PROJECT_NAME} PRIVATE Source/pch.h)

if (WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE shcore.lib xinput.lib)
endif()
